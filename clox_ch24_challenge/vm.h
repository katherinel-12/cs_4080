#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "object.h"
#include "value.h"
#include "table.h"

// maximum call depth able to handle
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

// CallFrame represents a single ongoing function call
// slots points into the VM’s value stack at the first slot that this function can use
typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

// virtual machine is one part of our interpreter’s internal architecture
// hand it a chunk of code and it runs it
typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    // the VM’s stack
    Value stack[STACK_MAX];
    Value* stackTop;
    Table globals;
    Table strings; // a hash table to store all strings so that "string == string" works (no duplicate keys in a table)
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