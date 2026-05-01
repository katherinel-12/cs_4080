#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "value.h"

// the maximum number of values we can store on the stack
#define STACK_MAX 256

// virtual machine is one part of our interpreter’s internal architecture
// hand it a chunk of code and it runs it
typedef struct {
    Chunk* chunk;
    // a byte pointer, ip points to the instruction about to be executed
    uint8_t* ip;
    // the VM’s stack
    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects; // a pointer to the head of the Obj list
} VM;

// VM runs the chunk and then responds with a value from this enum
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
  } InterpretResult;

extern VM vm;

void initVM();
void freeVM();

// the main entrypoint into the VM
// sets up a pipeline to scan, compile, and execute Lox source code
InterpretResult interpret(const char* source);

// stack protocol operations
void push(Value value);
Value pop();

#endif //CLOX_VM_H