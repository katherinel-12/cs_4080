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

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);

    // handling if allocation fails (not enough memory) and realloc() returns NULL
    if (result == NULL) exit(1);

    return result;
}

// walking a linked list and freeing its nodes
static void freeObject(Obj* object) {
    // // add back for Ch. 20
    // switch (object->type) {
    //     case OBJ_STRING: {
    //         ObjString* string = (ObjString*)object;
    //         FREE_ARRAY(char, string->chars, string->length + 1);
    //         FREE(ObjString, object);
    //         break;
    //     }
    // }

    // added for Ch. 19.1 challenge
    switch (object->type) {
        // case OBJ_STRING: {
        //     ObjString* string = (ObjString*)object;
        //     reallocate(object, sizeof(ObjString) + string->length + 1, 0);
        //     break;
        // }
        case OBJ_STRING: {
            ObjString* string = (ObjString*)object;
            if (string->ownsChars) { // <--
                FREE_ARRAY(char, (char*)string->chars, string->length + 1);
            }
            FREE(ObjString, object);
            break;
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
}