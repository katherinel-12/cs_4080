// #ifndef CLOX_VALUE_H
// #define CLOX_VALUE_H
//
// // in C, HAVE to build the data structures out
//
// #include <string.h>
// #include "common.h"
//
// // a struct that contains the state shared across all object types (string, instance, etc)
// typedef struct Obj Obj;
// // strings build on top of Obj
// typedef struct ObjString ObjString;
//
// #ifdef NAN_BOXING
//
// #define SIGN_BIT ((uint64_t)0x8000000000000000)
// #define QNAN     ((uint64_t)0x7ffc000000000000)
//
// #define TAG_NIL   1 // 01.
// #define TAG_FALSE 2 // 10.
// #define TAG_TRUE  3 // 11.
// #define TAG_SHORT_STR  4 // added for Ch. 30 challenge 2
//
// #define SHORT_STR_LEN_MASK 0x7 // added for Ch. 30 challenge 2
//
// // added for Ch. 30 challenge 2
// #define IS_SHORT_STR(v) \
// (((v) & (QNAN | SIGN_BIT | 0x7)) == (QNAN | TAG_SHORT_STR))
// #define AS_SHORT_STR_LEN(v) ((int)((v) & SHORT_STR_LEN_MASK))
// #define AS_SHORT_STR(v) ((char*)(&(v)) + 1)
//
// typedef uint64_t Value;
//
// #define IS_BOOL(value)      (((value) | 1) == TRUE_VAL)
// #define IS_NIL(value)       ((value) == NIL_VAL)
// #define IS_NUMBER(value)    (((value) & QNAN) != QNAN)
// // #define IS_OBJ(value) \
// // (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
// #define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
// #define AS_BOOL(value)      ((value) == TRUE_VAL)
// #define AS_NUMBER(value)    valueToNum(value)
// #define AS_OBJ(value) \
// ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
// #define BOOL_VAL(b)     ((b) ? TRUE_VAL : FALSE_VAL)
// #define FALSE_VAL       ((Value)(uint64_t)(QNAN | TAG_FALSE))
// #define TRUE_VAL        ((Value)(uint64_t)(QNAN | TAG_TRUE))
// #define NIL_VAL         ((Value)(uint64_t)(QNAN | TAG_NIL))
// #define NUMBER_VAL(num) numToValue(num)
// #define OBJ_VAL(obj) \
// (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))
//
// static inline double valueToNum(Value value) {
//     double num;
//     memcpy(&num, &value, sizeof(Value));
//     return num;
// }
//
// static inline Value numToValue(double num) {
//     Value value;
//     memcpy(&value, &num, sizeof(double));
//     return value;
// }
//
// // added for Ch. 30 challenge 2
// static inline Value SHORT_STR_VAL(const char* s, int len) {
//     // initialize with the NaN prefix and tag
//     Value v = QNAN | TAG_SHORT_STR;
//
//     // store the length in the bottom 3 bits
//     v |= (uint64_t)len;
//
//     // copy characters into the "middle" of the 64-bit value.
//     // offset the destination pointer by 1 byte to stay away
//     // from the length bits and the high NaN bits.
//     memcpy((char*)&v + 1, s, len);
//
//     return v;
// }
//
// #else
//
// // how the value representation can dynamically handle different types
// // tagged union - a value contains two parts: a type “tag”, a payload for the actual value
// // enum to store the type “tag”
// typedef enum {
//     VAL_BOOL,
//     VAL_NIL,
//     VAL_NUMBER,
//     VAL_OBJ // this is to store strings in Ch. 19
//   } ValueType;
//
// // struct with field for the type tag, and a field containing the union of all the underlying values
// typedef struct {
//     ValueType type;
//     union { // a payload for the actual value
//         bool boolean;
//         double number;
//         Obj* obj; // the payload for a string (Ch. 19) is a pointer to the heap memory
//     } as;
// } Value;
//
// /* instead of :
//  * Value a = NUMBER_VAL(5.0);
//  * Value b = NUMBER_VAL(10.0);
//  *
//  * Value result = a + b; // ERROR: You can't use '+' on a struct!
//  *
//  * have :
//  * Value a = NUMBER_VAL(5.0);
//  * Value b = NUMBER_VAL(10.0);
//  *
//  * // 1. Unpack the raw doubles using AS_NUMBER
//  * double numA = AS_NUMBER(a);
//  * double numB = AS_NUMBER(b);
//  *
//  * // 2. Perform the actual C math
//  * double sum = numA + numB;
//  *
//  * // 3. Wrap it back into a Value to put it back on the stack
//  * Value result = NUMBER_VAL(sum);
//  */
// #define IS_BOOL(value)    ((value).type == VAL_BOOL)
// #define IS_NIL(value)     ((value).type == VAL_NIL)
// #define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
// #define IS_OBJ(value)     ((value).type == VAL_OBJ)
//
// // to do anything with a Value, unpack it and get the C value back out
// #define AS_OBJ(value)     ((value).as.obj) // extracts the Obj pointer from the value
// #define AS_BOOL(value)    ((value).as.boolean)
// #define AS_NUMBER(value)  ((value).as.number)
//
// // mandatory conversion step so the VM always knows what type of data it is looking at
// // by checking the tag before it tries to read the bits
// // by checking the tag before it tries to read the bits
// // ex.  double a = 5.0;
// //      Value b = NUMBER_VAL(a);  instead of  Value b = a;
// #define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
// #define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
// #define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
// // takes a bare Obj pointer and wraps it in a full Value
// #define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})
//
// #endif
//
// // this struct wraps a pointer to an array
// // along with its allocated capacity and
// // the number of elements in use
// typedef struct {
//     int capacity;
//     int count;
//     Value* values;
// } ValueArray;
//
// bool valuesEqual(Value a, Value b);
// void initValueArray(ValueArray* array);
// void writeValueArray(ValueArray* array, Value value);
// void freeValueArray(ValueArray* array);
// void printValue(Value value);
//
// #endif //CLOX_VALUE_H

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include <string.h>
#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

/*
 * Reserve the low 4 bits:
 *
 * bit 3 = short-string tag
 * bits 0–2 = string length (0–7)
 *
 * This prevents the length bits from overwriting the tag.
 */
#define TAG_NIL        1
#define TAG_FALSE      2
#define TAG_TRUE       3
#define TAG_SHORT_STR  8

#define SHORT_STR_LEN_MASK 0x7

typedef uint64_t Value;

#define IS_BOOL(value) \
    (((value) | 1) == TRUE_VAL)

#define IS_NIL(value) \
    ((value) == NIL_VAL)

#define IS_NUMBER(value) \
    (((value) & QNAN) != QNAN)

#define IS_OBJ(value) \
    (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))

#define IS_SHORT_STR(value) \
    (((value) & (QNAN | 0xF)) == (QNAN | TAG_SHORT_STR))

#define AS_BOOL(value) \
    ((value) == TRUE_VAL)

#define AS_NUMBER(value) \
    valueToNum(value)

#define AS_OBJ(value) \
    ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define AS_SHORT_STR_LEN(value) \
    ((int)((value) & SHORT_STR_LEN_MASK))

#define BOOL_VAL(b) \
    ((b) ? TRUE_VAL : FALSE_VAL)

#define FALSE_VAL \
    ((Value)(uint64_t)(QNAN | TAG_FALSE))

#define TRUE_VAL \
    ((Value)(uint64_t)(QNAN | TAG_TRUE))

#define NIL_VAL \
    ((Value)(uint64_t)(QNAN | TAG_NIL))

#define NUMBER_VAL(num) \
    numToValue(num)

#define OBJ_VAL(obj) \
    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))


static inline double valueToNum(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}

static inline Value numToValue(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

/*
 * Safe helper for short strings.
 * Returns pointer to embedded chars.
 */
static inline const char* asShortStr(Value value) {
    return ((const char*)&value) + 1;
}

#define AS_SHORT_STR(value) \
    asShortStr(value)


/*
 * Layout:
 *
 * byte 0: low bits hold tag + length
 * bytes 1–7: characters
 *
 * Supports strings up to length 6 safely.
 */
static inline Value SHORT_STR_VAL(const char* s, int len) {
    Value value = QNAN | TAG_SHORT_STR;

    value |= (uint64_t)len;

    memcpy((char*)&value + 1, s, len);

    return value;
}

#else

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;

    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;

} Value;

#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJ(value)     ((value).type == VAL_OBJ)

#define AS_OBJ(value)     ((value).as.obj)
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)

#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

#endif


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

#endif