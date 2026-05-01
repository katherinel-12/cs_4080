#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

// in C, HAVE to build the data structures out

#include "common.h"

// a struct that contains the state shared across all object types (string, instance, etc)
typedef struct Obj Obj;
// strings build on top of Obj
typedef struct ObjString ObjString;

// how the value representation can dynamically handle different types
// tagged union - a value contains two parts: a type “tag”, a payload for the actual value
// enum to store the type “tag”
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ, // this is to store strings in Ch. 19
    VAL_EMPTY // added for ch 20.1 challenge, only for internal use to identify empty hash table buckets
  } ValueType;

// struct with field for the type tag, and a field containing the union of all the underlying values
typedef struct {
    ValueType type;
    union { // a payload for the actual value
        bool boolean;
        double number;
        Obj* obj; // the payload for a string (Ch. 19) is a pointer to the heap memory
    } as;
} Value;

/* instead of :
 * Value a = NUMBER_VAL(5.0);
 * Value b = NUMBER_VAL(10.0);
 *
 * Value result = a + b; // ERROR: You can't use '+' on a struct!
 *
 * have :
 * Value a = NUMBER_VAL(5.0);
 * Value b = NUMBER_VAL(10.0);
 *
 * // 1. Unpack the raw doubles using AS_NUMBER
 * double numA = AS_NUMBER(a);
 * double numB = AS_NUMBER(b);
 *
 * // 2. Perform the actual C math
 * double sum = numA + numB;
 *
 * // 3. Wrap it back into a Value to put it back on the stack
 * Value result = NUMBER_VAL(sum);
 */
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)
#define IS_EMPTY(value)   ((value).type == VAL_EMPTY) // added for ch 20.1 challenge

// to do anything with a Value, unpack it and get the C value back out
#define AS_OBJ(value)     ((value).as.obj) // extracts the Obj pointer from the value
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
#define EMPTY_VAL         ((Value){ VAL_EMPTY, { .number = 0 } }) // added for ch 20.1 challenge

// mandatory conversion step so the VM always knows what type of data it is looking at
// by checking the tag before it tries to read the bits
// ex.  double a = 5.0;
//      Value b = NUMBER_VAL(a);  instead of  Value b = a;
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
// takes a bare Obj pointer and wraps it in a full Value
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

// this struct wraps a pointer to an array
// along with its allocated capacity and
// the number of elements in use
typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

bool valuesEqual(Value a, Value b);
void initValueArray(ValueArray* array);
void writeValueArray(ValueArray* array, Value value);
void freeValueArray(ValueArray* array);
void printValue(Value value);

uint32_t hashValue(Value value); // added for ch 20.1 challenge

#endif //CLOX_VALUE_H