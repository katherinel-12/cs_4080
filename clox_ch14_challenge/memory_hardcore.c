#include "memory_hardcore.h"
#include <stdlib.h>
#include <string.h>

// added for Challenge 14.3

static unsigned char* heap = NULL;
static size_t heapSize = 0;
static size_t offset = 0;

bool hardcore_allocator_init(size_t size) {
    heap = (unsigned char*)malloc(size);
    if (!heap) return false;

    heapSize = size;
    offset = 0;
    return true;
}

void hardcore_allocator_shutdown(void) {
    free(heap);
    heap = NULL;
    heapSize = 0;
    offset = 0;
}

void* hardcore_reallocate(void* pointer, size_t oldSize, size_t newSize) {
    // FREE case
    if (newSize == 0) {
        // bump allocator does NOT reclaim memory
        return NULL;
    }

    // ALLOCATE new
    if (pointer == NULL) {
        if (offset + newSize > heapSize) {
            return NULL; // out of memory
        }

        void* result = heap + offset;
        offset += newSize;
        return result;
    }

    // REALLOCATE (grow/shrink)
    if (offset + newSize > heapSize) {
        return NULL;
    }

    void* newPtr = heap + offset;
    offset += newSize;

    // copy old data
    size_t copySize = oldSize < newSize ? oldSize : newSize;
    memcpy(newPtr, pointer, copySize);

    return newPtr;
}