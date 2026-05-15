// #include <stdio.h>
// #include <string.h>
//
// #include "memory.h"
// #include "object.h"
// #include "value.h"
// #include "vm.h"
// #include "table.h"
//
// #define ALLOCATE_OBJ(type, objectType) \
// (type*)allocateObject(sizeof(type), objectType)
//
// // allocates an object of the given size on the heap
// static Obj* allocateObject(size_t size, ObjType type) {
//     Obj* object = (Obj*)reallocate(NULL, 0, size);
//     object->type = type;
//
//     // every new object begins unmarked because still need to determine if it is reachable or not
//     object->isMarked = false;
//
//     // every time an Obj is allocated, insert it in the list
//     object->next = vm.objects;
//     vm.objects = object;
//
//     // every time the VM creates a new object, it’s tracked and logged for debugging
// #ifdef DEBUG_LOG_GC
//     printf("%p allocate %zu for %d\n", (void*)object, size, type);
// #endif
//
//     return object;
// }
//
// // constructor-like function stores the given closure and receiver
// ObjBoundMethod* newBoundMethod(Value receiver,
//                                ObjClosure* method) {
//     ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod,
//                                          OBJ_BOUND_METHOD);
//     bound->receiver = receiver;
//     bound->method = method;
//     return bound;
// }
//
// // takes in the class’s name as a string and stores it
// // klass because "class" is a reserved word in C
// ObjClass* newClass(ObjString* name) {
//     ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
//     klass->name = name;
//     initTable(&klass->methods); // when the memory manager deallocates a class, the table should be freed too
//     return klass;
// }
//
// ObjClosure* newClosure(ObjFunction* function) {
//     ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*,
//                                    function->upvalueCount);
//     for (int i = 0; i < function->upvalueCount; i++) {
//         upvalues[i] = NULL;
//     }
//
//     ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
//     closure->function = function;
//     closure->upvalues = upvalues;
//     closure->upvalueCount = function->upvalueCount;
//     return closure;
// }
//
// // use ALLOCATE_OBJ() to allocate memory and initialize the object’s header so the VM knows what type of object it is
// // set the function up in a blank state: 0 arity, no name, and no code
// ObjFunction* newFunction() {
//     ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
//     function->arity = 0;
//     function->upvalueCount = 0;
//     function->name = NULL;
//     initChunk(&function->chunk);
//     return function;
// }
//
// // store a reference to the instance’s class
// // then initialize the field table to an empty hash table
// ObjInstance* newInstance(ObjClass* klass) {
//     ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
//     instance->klass = klass;
//     initTable(&instance->fields);
//     return instance;
// }
//
// ObjNative* newNative(NativeFn function) {
//     ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
//     native->function = function;
//     return native;
// }
//
// // creates a new ObjString on the heap and then initializes its fields
// static ObjString* allocateString(char* chars, int length,
//                                  uint32_t hash) {
//     ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
//     string->length = length;
//     string->chars = chars;
//     string->hash = hash; // stores the hash in the struct
//
//     push(OBJ_VAL(string));
//     tableSet(&vm.strings, string, NIL_VAL); // automatically intern every string
//     pop();
//
//     return string;
// }
//
// // “hash function” in clox : FNV-1a algorithm
// // start with some initial hash value
// // for each byte/word, mix the bits into the hash value
// // then scramble the resulting bits around
// // scatter well to avoid collisions and clustering
// static uint32_t hashString(const char* key, int length) {
//     uint32_t hash = 2166136261u;
//     for (int i = 0; i < length; i++) {
//         hash ^= (uint8_t)key[i];
//         hash *= 16777619;
//     }
//     return hash;
// }
//
// // added for Ch. 30 challenge 2
// Value newString(const char* chars, int length) {
//     // check for the optimization first
//     if (length <= 6) {
//         return SHORT_STR_VAL(chars, length);
//     }
//
//     // if it's too big, fall back to the normal heap/intern logic
//     return OBJ_VAL(copyString(chars, length));
// }
//
// // for concatenation, already dynamically allocated character array on the heap
// // this function claims ownership of the string given
// // ObjString* takeString(char* chars, int length) {
// Value takeString(char* chars, int length) {
//     if (length <= 6) {
//         return SHORT_STR_VAL(chars, length);
//     }
//
//     uint32_t hash = hashString(chars, length);
//
//     // look up the string in the string table first
//     // find it, then free memory of the string passed in and then return string already there
//     ObjString* interned = tableFindString(&vm.strings, chars, length,
//                                         hash);
//     if (interned != NULL) {
//         FREE_ARRAY(char, chars, length + 1);
//         return OBJ_VAL(interned);
//     }
//
//     return OBJ_VAL(allocateString(chars, length, hash));
// }
//
// // // assumes it cannot take ownership of the characters
// // // creates a copy of the characters on the heap that the ObjString can own
// // // ObjString* copyString(const char* chars, int length) {
// // Value copyString(const char* chars, int length) {
// //     if (length <= 6) {
// //         return SHORT_STR_VAL(chars, length);
// //     }
// //
// //     uint32_t hash = hashString(chars, length);
// //
// //     // gets a string into the string table assuming that it’s unique
// //     // check for duplicates first
// //     ObjString* interned = tableFindString(&vm.strings, chars, length,
// //                                         hash);
// //     if (interned != NULL) return OBJ_VAL(interned);
// //
// //     char* heapChars = ALLOCATE(char, length + 1);
// //     memcpy(heapChars, chars, length);
// //     heapChars[length] = '\0';
// //
// //     return OBJ_VAL(allocateString(heapChars, length, hash));
// // }
//
// // this function ignores the optimization and always puts it on the heap
// static ObjString* copyStringOnHeap(const char* chars, int length) {
//     uint32_t hash = hashString(chars, length);
//     ObjString* interned = tableFindString(&vm.strings, chars, length, hash);
//     if (interned != NULL) return interned;
//
//     char* heapChars = ALLOCATE(char, length + 1);
//     memcpy(heapChars, chars, length);
//     heapChars[length] = '\0';
//
//     return allocateString(heapChars, length, hash);
// }
//
// Value copyString(const char* chars, int length) {
//     if (length <= 6) {
//         return SHORT_STR_VAL(chars, length);
//     }
//     // Wrap the pointer from our helper into a Value
//     return OBJ_VAL(copyStringOnHeap(chars, length));
// }
//
// // ObjString* asString(Value value) {
// //     if (IS_OBJ(value)) return AS_STRING(value);
// //
// //     return copyStringOnHeap(AS_SHORT_STR(value), AS_SHORT_STR_LEN(value));
// // }
//
// ObjString* asString(Value value) {
//     if (IS_OBJ(value)) return AS_STRING(value);
//
//     if (IS_SHORT_STR(value)) {
//         int length = AS_SHORT_STR_LEN(value);
//         char buffer[8];
//         memcpy(buffer, (char*)&value + 1, length);
//         buffer[length] = '\0';
//         return copyStringOnHeap(buffer, length);
//     }
//
//     return NULL;
// }
//
// ObjUpvalue* newUpvalue(Value* slot) {
//     ObjUpvalue* upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
//     upvalue->closed = NIL_VAL;
//     upvalue->location = slot;
//     upvalue->next = NULL;
//     return upvalue;
// }
//
// static void printFunction(ObjFunction* function) {
//     if (function->name == NULL) {
//         printf("<script>");
//         return;
//     }
//
//     printf("<fn %s>", function->name->chars);
// }
//
// // prints the string Obj character array as a C string
// void printObject(Value value) {
//     switch (OBJ_TYPE(value)) {
//         case OBJ_BOUND_METHOD:
//             printFunction(AS_BOUND_METHOD(value)->method->function);
//             break;
//         case OBJ_CLASS:
//             printf("%s", AS_CLASS(value)->name->chars);
//             break;
//         case OBJ_CLOSURE:
//             printFunction(AS_CLOSURE(value)->function);
//             break;
//         case OBJ_FUNCTION:
//             printFunction(AS_FUNCTION(value));
//             break;
//         case OBJ_INSTANCE:
//             printf("%s instance",
//                    AS_INSTANCE(value)->klass->name->chars);
//             break;
//         case OBJ_NATIVE:
//             printf("<native fn>");
//             break;
//         case OBJ_STRING:
//             printf("%s", AS_CSTRING(value));
//             break;
//         case OBJ_UPVALUE:
//             printf("upvalue");
//             break;
//     }
// }

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);

    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n",
           (void*)object, size, type);
#endif

    return object;
}

/************ Constructors ************/

ObjBoundMethod* newBoundMethod(
    Value receiver,
    ObjClosure* method
) {
    ObjBoundMethod* bound =
        ALLOCATE_OBJ(
            ObjBoundMethod,
            OBJ_BOUND_METHOD
        );

    bound->receiver = receiver;
    bound->method = method;

    return bound;
}

ObjClass* newClass(
    ObjString* name
) {
    ObjClass* klass =
        ALLOCATE_OBJ(
            ObjClass,
            OBJ_CLASS
        );

    klass->name = name;

    initTable(
        &klass->methods
    );

    return klass;
}

ObjClosure* newClosure(
    ObjFunction* function
) {
    ObjUpvalue** upvalues =
        ALLOCATE(
            ObjUpvalue*,
            function->upvalueCount
        );

    for (
        int i = 0;
        i < function->upvalueCount;
        i++
    ) {
        upvalues[i] = NULL;
    }

    ObjClosure* closure =
        ALLOCATE_OBJ(
            ObjClosure,
            OBJ_CLOSURE
        );

    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount =
        function->upvalueCount;

    return closure;
}

ObjFunction* newFunction() {
    ObjFunction* function =
        ALLOCATE_OBJ(
            ObjFunction,
            OBJ_FUNCTION
        );

    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;

    initChunk(
        &function->chunk
    );

    return function;
}

ObjInstance* newInstance(
    ObjClass* klass
) {
    ObjInstance* instance =
        ALLOCATE_OBJ(
            ObjInstance,
            OBJ_INSTANCE
        );

    instance->klass = klass;

    initTable(
        &instance->fields
    );

    return instance;
}

ObjNative* newNative(
    NativeFn function
) {
    ObjNative* native =
        ALLOCATE_OBJ(
            ObjNative,
            OBJ_NATIVE
        );

    native->function = function;

    return native;
}

ObjUpvalue* newUpvalue(
    Value* slot
) {
    ObjUpvalue* upvalue =
        ALLOCATE_OBJ(
            ObjUpvalue,
            OBJ_UPVALUE
        );

    upvalue->location = slot;
    upvalue->closed = NIL_VAL;
    upvalue->next = NULL;

    return upvalue;
}

/************ String handling ************/

static uint32_t hashString(
    const char* key,
    int length
) {
    uint32_t hash =
        2166136261u;

    for (
        int i = 0;
        i < length;
        i++
    ) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }

    return hash;
}

static ObjString* allocateString(
    char* chars,
    int length,
    uint32_t hash
) {
    ObjString* string =
        ALLOCATE_OBJ(
            ObjString,
            OBJ_STRING
        );

    string->chars = chars;
    string->length = length;
    string->hash = hash;

    push(
        OBJ_VAL(string)
    );

    tableSet(
        &vm.strings,
        string,
        NIL_VAL
    );

    pop();

    return string;
}

static ObjString* copyStringOnHeap(
    const char* chars,
    int length
) {
    uint32_t hash =
        hashString(
            chars,
            length
        );

    ObjString* interned =
        tableFindString(
            &vm.strings,
            chars,
            length,
            hash
        );

    if (
        interned != NULL
    ) {
        return interned;
    }

    char* heapChars =
        ALLOCATE(
            char,
            length + 1
        );

    memcpy(
        heapChars,
        chars,
        length
    );

    heapChars[length] = '\0';

    return allocateString(
        heapChars,
        length,
        hash
    );
}

Value copyString(
    const char* chars,
    int length
) {
    if (
        length <= 6
    ) {
        return SHORT_STR_VAL(
            chars,
            length
        );
    }

    return OBJ_VAL(
        copyStringOnHeap(
            chars,
            length
        )
    );
}

Value takeString(
    char* chars,
    int length
) {
    if (
        length <= 6
    ) {
        return SHORT_STR_VAL(
            chars,
            length
        );
    }

    uint32_t hash =
        hashString(
            chars,
            length
        );

    ObjString* interned =
        tableFindString(
            &vm.strings,
            chars,
            length,
            hash
        );

    if (
        interned != NULL
    ) {
        FREE_ARRAY(
            char,
            chars,
            length + 1
        );

        return OBJ_VAL(
            interned
        );
    }

    return OBJ_VAL(
        allocateString(
            chars,
            length,
            hash
        )
    );
}

ObjString* asString(
    Value value
) {
    if (
        IS_OBJ(value)
    ) {
        return AS_STRING(
            value
        );
    }

    if (
        IS_SHORT_STR(value)
    ) {
        int length =
            AS_SHORT_STR_LEN(
                value
            );

        char buffer[8];

        memcpy(
            buffer,
            AS_SHORT_STR(
                value
            ),
            length
        );

        buffer[length] =
            '\0';

        return copyStringOnHeap(
            buffer,
            length
        );
    }

    return NULL;
}

/************ Printing ************/

static void printFunction(
    ObjFunction* function
) {
    if (
        function->name == NULL
    ) {
        printf(
            "<script>"
        );

        return;
    }

    printf(
        "<fn %s>",
        function->name->chars
    );
}

void printObject(
    Value value
) {
    switch (
        OBJ_TYPE(value)
    ) {

        case OBJ_BOUND_METHOD:
            printFunction(
                AS_BOUND_METHOD(
                    value
                )->method->function
            );
            break;

        case OBJ_CLASS:
            printf(
                "%s",
                AS_CLASS(
                    value
                )->name->chars
            );
            break;

        case OBJ_CLOSURE:
            printFunction(
                AS_CLOSURE(
                    value
                )->function
            );
            break;

        case OBJ_FUNCTION:
            printFunction(
                AS_FUNCTION(
                    value
                )
            );
            break;

        case OBJ_INSTANCE:
            printf(
                "%s instance",
                AS_INSTANCE(
                    value
                )->klass->name->chars
            );
            break;

        case OBJ_NATIVE:
            printf(
                "<native fn>"
            );
            break;

        case OBJ_STRING:
            printf(
                "%s",
                AS_CSTRING(
                    value
                )
            );
            break;

        case OBJ_UPVALUE:
            printf(
                "upvalue"
            );
            break;
    }
}