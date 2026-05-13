#ifndef CLOX_OBJECT_H
#define CLOX_OBJECT_H
//
// #include "common.h"
// #include "value.h"
// #include "chunk.h"
// #include "table.h"
//
// // macro that extracts the object type tag from a given Value
// #define OBJ_TYPE(value)        (AS_OBJ(value)->type)
//
// #define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
// #define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
// #define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
// // macro for converting values to functions
// // make sure the value actually is a function
// #define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
// #define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
// #define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
// // this is kind of OOP, so ObjString* can cast to Obj*
// // this macro checks that Obj* can be downcast to ObjString*
// // takes a Value for VM sake
// #define IS_STRING(value)       isObjType(value, OBJ_STRING)
//
// #define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
// #define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
// #define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
// // cast the Value to an ObjFunction pointer
// #define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
// #define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
// #define AS_NATIVE(value) \
// (((ObjNative*)AS_OBJ(value))->function)
// // macro telling us when it’s safe to cast a value to a specific object type
// // take a Value that is expected to contain a pointer to a valid ObjString on the heap
// // first returns the ObjString* pointer
// // second returns the character array itself
// #define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
// #define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)
//
// typedef enum {
//     OBJ_BOUND_METHOD,
//     OBJ_CLASS,
//     OBJ_CLOSURE,
//     OBJ_FUNCTION,
//     OBJ_INSTANCE,
//     OBJ_NATIVE,
//     OBJ_STRING,
//     OBJ_UPVALUE
//   } ObjType;
//
// // Obj becomes a linked list to store every Obj
// struct Obj {
//     ObjType type;
//     bool isMarked;
//     struct Obj* next; // avoids leaking memory
//     // VM can traverse the list to find every object allocated on the heap
// };
//
// // give each function its own Chunk
// // arity stores the number of parameters the function expects
// // name is the function name for reporting readable runtime errors
// typedef struct {
//     Obj obj;
//     int arity;
//     int upvalueCount;
//     Chunk chunk;
//     ObjString* name;
// } ObjFunction;
//
// // native functions are an entirely different object type
// typedef Value (*NativeFn)(int argCount, Value* args);
//
// typedef struct {
//     Obj obj;
//     NativeFn function; // pointer to the C function that implements the native behavior
// } ObjNative;
//
// // the payload for strings
// struct ObjString {
//     Obj obj;
//     int length;
//     char* chars;
//     uint32_t hash; // stores the hash code for its string
// };
//
// typedef struct ObjUpvalue {
//     Obj obj;
//     Value* location;
//     Value closed;
//     struct ObjUpvalue* next;
// } ObjUpvalue;
//
// // // wrap every function in an ObjClosure, even if the function doesn’t close over and capture any surrounding local variables
// // // this simplifies the VM
// // typedef struct {
// //     Obj obj;
// //     ObjFunction* function;
// //     ObjUpvalue** upvalues;
// //     int upvalueCount;
// // } ObjClosure;
//
// // added for Ch. 29 challenge
// typedef struct {
//     Obj obj;
//     ObjFunction* function;
//     ObjUpvalue** upvalues;
//     int upvalueCount;
//     struct ObjClass* owner; // Tag the closure
// } ObjClosure;
//
// // typedef struct {
// //     Obj obj;
// //     ObjString* name;
// //     Table methods;
// // } ObjClass;
//
// // added for Ch. 29 challenge
// typedef struct sObjClass {
//     Obj obj;
//     ObjString* name;
//     Table methods;      // Inherited methods
//     Table ownMethods;   // Methods defined only in this class
//     struct ObjClass* superclass;
// } ObjClass;
//
// typedef struct {
//     Obj obj;
//     ObjClass* klass;
//     Table fields;
// } ObjInstance;
//
// // wraps the receiver and the method closure together
// typedef struct {
//     Obj obj;
//     Value receiver;
//     ObjClosure* method;
// } ObjBoundMethod;
//
// ObjBoundMethod* newBoundMethod(Value receiver,
//                                ObjClosure* method);
//
// // the VM creates new class objects using this function
// ObjClass* newClass(ObjString* name);
//
// // Ch. 27 object-oriented programming - class objects
// // Ch. 27 will focus on classes, instances, and fields
// // out of classes, instances, fields, methods, initializers, and inheritance
// ObjClosure* newClosure(ObjFunction* function);
//
// // a C function to create a new Lox function
// ObjFunction* newFunction();
//
// // since fields are added after the instance is created,
// // the “constructor” function only needs to know the class
// ObjInstance* newInstance(ObjClass* klass);
//
// // to create an ObjNative
// ObjNative* newNative(NativeFn function);
//
// ObjString* takeString(char* chars, int length);
//
// // to create the string
// ObjString* copyString(const char* chars, int length);
//
// ObjUpvalue* newUpvalue(Value* slot);
//
// // if the Value being printed in value.c is a heap-allocated object
// // it'll use this function
// void printObject(Value value);
//
// static inline bool isObjType(Value value, ObjType type) {
//     return IS_OBJ(value) && AS_OBJ(value)->type == type;
// }
//

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value)        (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)        isObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)      isObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)     isObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)     isObjType(value, OBJ_INSTANCE)
#define IS_NATIVE(value)       isObjType(value, OBJ_NATIVE)
#define IS_STRING(value)       isObjType(value, OBJ_STRING)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod*)AS_OBJ(value))
#define AS_CLASS(value)        ((ObjClass*)AS_OBJ(value))
#define AS_CLOSURE(value)      ((ObjClosure*)AS_OBJ(value))
#define AS_FUNCTION(value)     ((ObjFunction*)AS_OBJ(value))
#define AS_INSTANCE(value)     ((ObjInstance*)AS_OBJ(value))
#define AS_NATIVE(value)       (((ObjNative*)AS_OBJ(value))->function)
#define AS_STRING(value)       ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value)      (((ObjString*)AS_OBJ(value))->chars)

typedef enum {
  OBJ_BOUND_METHOD,
  OBJ_CLASS,
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_INSTANCE,
  OBJ_NATIVE,
  OBJ_STRING
} ObjType;

struct Obj {
  ObjType type;
  bool isMarked;
  struct Obj* next;
};

typedef struct Obj Obj;

typedef struct {
  Obj obj;
  int arity;
  int upvalueCount;
  Chunk chunk;
  ObjString* name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

typedef struct ObjString {
  Obj obj;
  int length;
  char* chars;
  uint32_t hash;
} ObjString;

typedef struct ObjUpvalue {
  Obj obj;
  Value* location;
  Value closed;
  struct ObjUpvalue* next;
} ObjUpvalue;

struct ObjClass;

typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues;
  int upvalueCount;

  // BETA: class that defined this method.
  struct ObjClass* owner;

} ObjClosure;

typedef struct ObjClass {
  Obj obj;
  ObjString* name;

  // Dispatch table.
  Table methods;

  // Only methods defined directly on this class.
  Table ownMethods;

  struct ObjClass* superclass;

} ObjClass;

typedef struct {
  Obj obj;
  ObjClass* klass;
  Table fields;
} ObjInstance;

typedef struct {
  Obj obj;
  Value receiver;
  ObjClosure* method;
} ObjBoundMethod;

ObjClass* newClass(ObjString* name);
ObjClosure* newClosure(ObjFunction* function);
ObjFunction* newFunction(void);
ObjInstance* newInstance(ObjClass* klass);
ObjNative* newNative(NativeFn function);
ObjString* takeString(char* chars, int length);
ObjString* copyString(const char* chars, int length);
ObjUpvalue* newUpvalue(Value* slot);
ObjBoundMethod* newBoundMethod(Value receiver, ObjClosure* method);

void printObject(Value value);

#endif //CLOX_OBJECT_H