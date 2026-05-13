#ifndef CLOX_COMPILER_H
#define CLOX_COMPILER_H

#include "vm.h"
#include "object.h"

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
    TYPE_METHOD,
    TYPE_INITIALIZER
  } FunctionType;

ObjFunction* compile(const char* source);

void markCompilerRoots();

#endif //CLOX_COMPILER_H