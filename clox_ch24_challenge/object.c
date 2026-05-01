#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "table.h"

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

// use ALLOCATE_OBJ() to allocate memory and initialize the object’s header so the VM knows what type of object it is
// set the function up in a blank state: 0 arity, no name, and no code
ObjFunction* newFunction() {
    ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjNative* newNative(NativeFn function, int arity) {
    ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    native->arity = arity; // added for ch 24.2 challenge
    return native;
}

// creates a new ObjString on the heap and then initializes its fields
static ObjString* allocateString(char* chars, int length,
                                 uint32_t hash) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash; // stores the hash in the struct
    tableSet(&vm.strings, string, NIL_VAL); // automatically intern every string

    return string;
}

// “hash function” in clox : FNV-1a algorithm
// start with some initial hash value
// for each byte/word, mix the bits into the hash value
// then scramble the resulting bits around
// scatter well to avoid collisions and clustering
static uint32_t hashString(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

// for concatenation, already dynamically allocated character array on the heap
// this function claims ownership of the string given
ObjString* takeString(char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    // look up the string in the string table first
    // find it, then free memory of the string passed in and then return string already there
    ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocateString(chars, length, hash);
}
// assumes it cannot take ownership of the characters
// creates a copy of the characters on the heap that the ObjString can own
ObjString* copyString(const char* chars, int length) {
    uint32_t hash = hashString(chars, length);

    // gets a string into the string table assuming that it’s unique
    // check for duplicates first
    ObjString* interned = tableFindString(&vm.strings, chars, length,
                                        hash);
    if (interned != NULL) return interned;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->chars);
}

// prints the string Obj character array as a C string
void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}