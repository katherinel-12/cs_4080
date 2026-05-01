#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"

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

#endif //CLOX_MEMORY_H