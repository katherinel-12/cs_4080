#ifndef CLOX_MEMORY_HARDCORE_H
#define CLOX_MEMORY_HARDCORE_H

// added for Challenge 14.3
#include <stddef.h>
#include <stdbool.h>

bool hardcore_allocator_init(size_t size);
void hardcore_allocator_shutdown(void);

void* hardcore_reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif //CLOX_MEMORY_HARDCORE_H