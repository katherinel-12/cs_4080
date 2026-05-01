#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "value.h"

// commented out for Ch. 15.3
// // the maximum number of values we can store on the stack
// #define STACK_MAX 256

// commented out for Ch. 15.3
// // virtual machine is one part of our interpreter’s internal architecture
// // hand it a chunk of code and it runs it
// typedef struct {
//     Chunk* chunk;
//     // a byte pointer, ip points to the instruction about to be executed
//     uint8_t* ip;
//     // the VM’s stack
//     Value stack[STACK_MAX];
//     Value* stackTop;
// } VM;

// added for Ch. 15.3
typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value* stack;
    int stackCount;
    int stackCapacity;
} VM;

// VM runs the chunk and then responds with a value from this enum
typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
  } InterpretResult;

void initVM();
void freeVM();

// the main entrypoint into the VM
InterpretResult interpret(Chunk* chunk);

// stack protocol operations
void push(Value value);
Value pop();

#endif //CLOX_VM_H