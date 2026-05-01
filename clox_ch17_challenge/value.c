#include <stdio.h>

#include "memory.h"
#include "value.h"

// create an initialized array
void initValueArray(ValueArray* array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

// have an initialized array and start adding values to it

void writeValueArray(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values = GROW_ARRAY(Value, array->values,
                                   oldCapacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

// release all memory used by the array
void freeValueArray(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

// print a clox Value
void printValue(Value value) {
    printf("%g", AS_NUMBER(value)); // replaces : printf("%g", value); after Value becomes struct
}

