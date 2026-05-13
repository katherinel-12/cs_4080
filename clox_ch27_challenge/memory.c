/*
oldSize 	newSize 	            Operation
0 	        Non‑zero 	            Allocate new block.
Non‑zero 	0 	                    Free allocation.
Non‑zero 	Smaller than oldSize 	Shrink existing allocation.
Non‑zero 	Larger than oldSize 	Grow existing allocation.
*/

#include <stdlib.h>

#include "memory.h"
#include "vm.h"
#include "object.h"
#include "compiler.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

// from Ch. 26, whenever calling reallocate() to acquire more memory,
// force a garbage collection to run
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif
        if (vm.bytesAllocated > vm.nextGC) {
            collectGarbage();
        }
    }

    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);

    // handling if allocation fails (not enough memory) and realloc() returns NULL
    if (result == NULL) exit(1);

    return result;
}

void markObject(Obj* object) {
    if (object == NULL) return;
    // to ensure the garbage collector doesn’t get stuck in a graph's infinite loop
    // ensures that an already-gray object is not added again
    // and that a black object is not turned back to gray
    if (object->isMarked) return;

    // logging: see what the mark phase is doing
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif

    object->isMarked = true;

    if (vm.grayCapacity < vm.grayCount + 1) {
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        vm.grayStack = (Obj**)realloc(vm.grayStack,
                                      sizeof(Obj*) * vm.grayCapacity);

        if (vm.grayStack == NULL) exit(1);
    }

    vm.grayStack[vm.grayCount++] = object;
}

// ensure that the value is a heap object
// numbers, Booleans, and nil are stored directly inline in Value and require no heap allocation
void markValue(Value value) {
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

// OBJ_FUNCTION in blackenObject() has a constant table packed with references to other objects
// trace all of those references with this helper
static void markArray(ValueArray* array) {
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj* object) {
    // logging: watch the tracing go through the object graph
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif
    switch (object->type) {
        case OBJ_CLASS: {
            ObjClass* klass = (ObjClass*)object;
            markObject((Obj*)klass->name);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure->function);
            for (int i = 0; i < closure->upvalueCount; i++) {
                markObject((Obj*)closure->upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            markObject((Obj*)function->name);
            markArray(&function->chunk.constants);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            markObject((Obj*)instance->klass);
            markTable(&instance->fields);
            break;
        }
        case OBJ_UPVALUE:
            markValue(((ObjUpvalue*)object)->closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

// walking a linked list and freeing its nodes
static void freeObject(Obj* object) {
    // // log the memory address and type of the object being freed for GC debugging
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif

    switch (object->type) {
        case OBJ_CLASS: {
            FREE(ObjClass, object);
            break;
        }
        case OBJ_CLOSURE: {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues,
                       closure->upvalueCount);
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION: {
            ObjFunction* function = (ObjFunction*)object;
            freeChunk(&function->chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_INSTANCE: {
            ObjInstance* instance = (ObjInstance*)object;
            freeTable(&instance->fields);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
    }
}

// walk the roots,
// most roots are local variables or temporaries in the VM’s stack
static void markRoots() {
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frameCount; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm.openUpvalues;
       upvalue != NULL;
       upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
       }

    // another source of roots is the global variables
    markTable(&vm.globals);

    markCompilerRoots();
}

/*
* until the stack empties, keep pulling out gray objects
* traverse the grey objects' references, and then mark them black
*
* traversing an object’s references may turn up new white objects
* those white objects get marked gray and added to the stack
*
* this function swings back and forth between turning white objects gray
* and gray objects black
 */
static void traceReferences() {
    while (vm.grayCount > 0) {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

// outer while loop walks the linked list of every object in the heap, checking their mark bits
// if an object is marked (black), leave it alone and continue
// if it is unmarked (white), unlink it from the list and free it with the freeObject() function
static void sweep() {
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->isMarked) {
            // when the NEXT collection cycle starts, need every object to be white
            // so whenever reach a black object, go ahead and clear the bit now for the next run
            object->isMarked = false;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void freeObjects() {
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    // free is the stack container used to track the grey reachables when the VM shuts down
    free(vm.grayStack);
}

/*
 * Throughput: total fraction of time spent running user code versus doing garbage collection work
 * ex. run a program for 10 seconds and it spends 1 second of that inside collectGarbage()
 * throughput: spent 90% of the time running the program and 10% on GC overhead
 *
 * Latency: longest continuous chunk of time where the user’s program is completely paused for garbage collection
 * ex. 2 runs of a clox program that both take 10 seconds
 * 1st run, the GC kicks in once and spends 1 second in collectGarbage() in one massive collection
 * 2nd run, the GC gets invoked 5 times, each for 1/5 of a second
 * The total amount of time spent collecting is still 1 second,
 * the throughput is 90% in both cases. In the 2nd run, the latency is five times less than in the first
 *
 * ex. program is a bakery selling bread
 * throughput is the total number of bread can serve in a single day
 * latency is how long the unluckiest customer has to wait before they get served
 * running the GC is shutting down the bakery temporarily to go through all the dishes,
 * sort the dirty from the clean, and wash the used dishes
 *
 * this collector is a stop-the-world GC - the user’s program is paused until the entire GC process is complete
 */
void collectGarbage() {
    // logging: to know when a collection run starts
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    // capture the heap size before the collection
    size_t before = vm.bytesAllocated;
#endif

    markRoots();

    // done marking the roots, now processing gray objects
    traceReferences();

    // remove references to unreachable strings
    tableRemoveWhite(&vm.strings);

    // anything still white never got touched by the trace and is garbage
    sweep();

    // adjust the threshold of the next GC
    // as the amount of memory the program uses grows, the threshold moves farther out
    // to limit the total time spent re-traversing the larger live set
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;

    // logging: to know when collection is done
#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    // print the results of the heap size
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytesAllocated, before, vm.bytesAllocated,
         vm.nextGC);
#endif
}