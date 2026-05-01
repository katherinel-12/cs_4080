#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"
#include "object.h"

// allocate a new array with given element type and count on the heap
// for strings in Ch. 19
#define ALLOCATE(type, count) \
(type*)reallocate(NULL, 0, sizeof(type) * (count))

// free the character array and then free the ObjString
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// get the capacity for the new array
#define GROW_CAPACITY(capacity) \
((capacity) < 8 ? 8 : (capacity) * 2)

// grow the array to that new capacity
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
(type*)reallocate(pointer, sizeof(type) * (oldCount), \
sizeof(type) * (newCount))

// frees the memory by passing in zero for the new size
#define FREE_ARRAY(type, pointer, oldCount) \
reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

void freeObjects();

#endif //CLOX_MEMORY_H