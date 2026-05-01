#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "vm.h"
#include "object.h"

// pass in the chunk where the compiler will write the code
// then compile() returns whether or not compilation succeeded
bool compile(const char* source, Chunk* chunk);

#endif //CLOX_COMPILER_H