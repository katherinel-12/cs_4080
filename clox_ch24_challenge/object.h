#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <stdbool.h>

#include "common.h"
#include "value.h"
#include "chunk.h"

// macro that extracts the object type tag from a given Value
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
// macro for converting values to functions
// make sure the value actually is a function
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
// this is kind of OOP, so ObjString* can cast to Obj*
// this macro checks that Obj* can be downcast to ObjString*
// takes a Value for VM sake
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
// cast the Value to an ObjFunction pointer
#define AS_FUNCTION(value)    ((ObjFunction*)AS_OBJ(value))

// 1. Get the pointer to the whole struct
#define AS_NATIVE_OBJ(value)  ((ObjNative*)AS_OBJ(value))

// 2. Get just the C function pointer (for legacy use or convenience)
#define AS_NATIVE_FN(value)   (AS_NATIVE_OBJ(value)->function)
// macro telling us when it’s safe to cast a value to a specific object type
// take a Value that is expected to contain a pointer to a valid ObjString on the heap
// first returns the ObjString* pointer
// second returns the character array itself
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
  } ObjType;

// added for ch 24.3 challenge
typedef struct {
    Value value;
    bool hasError;
} NativeResult;

// Obj becomes a linked list to store every Obj
struct Obj {
    ObjType type;
    struct Obj* next; // avoids leaking memory
    // VM can traverse the list to find every object allocated on the heap
};

// give each function its own Chunk
// arity stores the number of parameters the function expects
// name is the function name for reporting readable runtime errors
typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

// // native functions are an entirely different object type
// typedef Value (*NativeFn)(int argCount, Value* args);

// added for ch 24.3 challenge
typedef NativeResult (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function; // pointer to the C function that implements the native behavior
    int arity; // added for ch 24.2 challenge
} ObjNative;

// the payload for strings
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash; // stores the hash code for its string
};

// a C function to create a new Lox function
ObjFunction* newFunction();

// to create an ObjNative
// added for ch 24.3 challenge
ObjNative* newNative(NativeFn function, int arity);

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