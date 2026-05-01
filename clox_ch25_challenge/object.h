#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H

#include <stdbool.h>

#include "common.h"
#include "value.h"
#include "chunk.h"

// macro that extracts the object type tag from a given Value
#define OBJ_TYPE(value)        (AS_OBJ(value)->type)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
// macro for converting values to functions
// make sure the value actually is a function
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
// this is kind of OOP, so ObjString* can cast to Obj*
// this macro checks that Obj* can be downcast to ObjString*
// takes a Value for VM sake
#define IS_STRING(value)       isObjType(value, OBJ_STRING)
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
// cast the Value to an ObjFunction pointer
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_NATIVE(value) \
(((ObjNative*)AS_OBJ(value))->function)
// macro telling us when it’s safe to cast a value to a specific object type
// take a Value that is expected to contain a pointer to a valid ObjString on the heap
// first returns the ObjString* pointer
// second returns the character array itself
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE
  } ObjType;

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
    int upvalueCount;
    Chunk chunk;
    ObjString* name;
} ObjFunction;

// native functions are an entirely different object type
typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    NativeFn function; // pointer to the C function that implements the native behavior
} ObjNative;

// the payload for strings
struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash; // stores the hash code for its string
};

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

// wrap every function in an ObjClosure, even if the function doesn’t close over and capture any surrounding local variables
// this simplifies the VM
typedef struct {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int upvalueCount;
} ObjClosure;

ObjClosure* newClosure(ObjFunction* function);

// a C function to create a new Lox function
ObjFunction* newFunction();

// to create an ObjNative
ObjNative* newNative(NativeFn function);

ObjString* takeString(char* chars, int length);

// to create the string
ObjString* copyString(const char* chars, int length);

ObjUpvalue* newUpvalue(Value* slot);

// if the Value being printed in value.c is a heap-allocated object
// it'll use this function
void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif //CLOX_OBJECT_H