/*
oldSize 	newSize 	            Operation
0 	        Non‑zero 	            Allocate new block.
Non‑zero 	0 	                    Free allocation.
Non‑zero 	Smaller than oldSize 	Shrink existing allocation.
Non‑zero 	Larger than oldSize 	Grow existing allocation.
*/

#include <stdlib.h>

#include "memory.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);

    // handling if allocation fails (not enough memory) and realloc() returns NULL
    if (result == NULL) exit(1);

    return result;
}