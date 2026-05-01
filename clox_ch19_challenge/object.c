#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

// creating a string object

#define ALLOCATE_OBJ(type, objectType) \
(type*)allocateObject(sizeof(type), objectType)

// allocates an object of the given size on the heap
static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    // every time an Obj is allocated, insert it in the list
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

// creates a new ObjString on the heap and then initializes its fields
static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    // // add back for Ch. 20
    // string->chars = chars;

    // added for Ch. 19.1 challenge
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';

    return string;
}

// add back for Ch. 20
// // for concatenation, already dynamically allocated character array on the heap
// // this function claims ownership of the string given
// ObjString* takeString(char* chars, int length) {
//     return allocateString(chars, length);
// }
// // assumes it cannot take ownership of the characters
// // creates a copy of the characters on the heap that the ObjString can own
// ObjString* copyString(const char* chars, int length) {
//     char* heapChars = ALLOCATE(char, length + 1);
//     memcpy(heapChars, chars, length);
//     heapChars[length] = '\0';
//     return allocateString(heapChars, length);
// }

// // added for Ch. 19.1 challenge
// ObjString* makeString(int length) {
//     ObjString* string = (ObjString*)allocateObject(
//         sizeof(ObjString) + length + 1, OBJ_STRING);
//     string->length = length;
//     return string;
// }

// added for Ch. 19.1 challenge
// ObjString* copyString(const char* chars, int length) {
//     ObjString* string = makeString(length);
//
//     memcpy(string->chars, chars, length);
//     string->chars[length] = '\0';
//
//     printf("ObjString at: %p\n", (void*)string);
//     printf("chars at:     %p\n", (void*)string->chars);
//     printf("expected:     %p\n",
//            (void*)((char*)string + sizeof(ObjString)));
//
//     return string;
// }

// added for Ch. 19.2 challenge
ObjString* makeString(bool ownsChars, const char* chars, int length) {
    // 1. Allocate ONLY the size of the struct
    ObjString* string = (ObjString*)allocateObject(
        sizeof(ObjString), OBJ_STRING);

    // 2. Assign the fields
    string->length = length;
    string->chars = chars;     // This works now because chars is a pointer!
    string->ownsChars = ownsChars;

    return string;
}

// add back for Ch. 20
// // prints the string Obj character array as a C string
// void printObject(Value value) {
//     switch (OBJ_TYPE(value)) {
//         case OBJ_STRING:
//             printf("%s", AS_CSTRING(value));
//             break;
//     }
// }

// added for Ch. 19.2 challenge
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            // Changed:
            printf("%.*s", AS_STRING(value)->length, AS_CSTRING(value));
            break;
    }
}