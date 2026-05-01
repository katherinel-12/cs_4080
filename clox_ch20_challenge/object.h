#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <stdbool.h>

#include "common.h"
#include "value.h"

// macro that extracts the object type tag from a given Value
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
// this is kind of OOP, so ObjString* can cast to Obj*
// this macro checks that Obj* can be downcast to ObjString*
// takes a Value for VM sake
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
// macro telling us when it’s safe to cast a value to a specific object type
// take a Value that is expected to contain a pointer to a valid ObjString on the heap
// first returns the ObjString* pointer
// second returns the character array itself
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_STRING,
  } ObjType;

// Obj becomes a linked list to store every Obj
struct Obj {
    ObjType type;
    struct Obj* next; // avoids leaking memory
    // VM can traverse the list to find every object allocated on the heap
};

// the payload for strings
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash; // stores the hash code for its string
};

ObjString* takeString(char* chars, int length);

// to create the string
ObjString* copyString(const char* chars, int length);

// if the Value being printed in value.c is a heap-allocated object
// it'll use this function
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJECT_H