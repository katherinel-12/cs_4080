#include <stdio.h>

#include "common.h"
#include "vm.h"
#include "debug.h"
#include "value.h"
#include "memory.h"

VM vm;

// commented out for Ch. 15.3
// static void resetStack() {
//     // stackTop points to the beginning of the array to indicate the stack is empty
//     vm.stackTop = vm.stack;
// }

// added for Ch. 15.3
static void resetStack() {
    vm.stackCount = 0;
}

// commented out for Ch. 15.3
// // to create a VM
// void initVM() {
//     resetStack();
// }

// added for Ch. 15.3
void initVM() {
    vm.stack = NULL;
    vm.stackCapacity = 0;
    resetStack();
}

// commented out for Ch. 15.3
// to tear down a VM
void freeVM() {
}

// // added for Ch. 15.3
// void freeVM() {
//     free(vm.stack);
// }

// commented out for Ch. 15.3
// // push a new value onto the top of the stack
// void push(Value value) {
//     // stores value in the array element at the top of the stack
//     *vm.stackTop = value;
//     // increment the pointer to point to the next unused slot in the array
//     vm.stackTop++;
// }

// added for Ch. 15.3
void push(Value value) {
    if (vm.stackCapacity < vm.stackCount + 1) {
        int oldCapacity = vm.stackCapacity;
        vm.stackCapacity = GROW_CAPACITY(oldCapacity);
        vm.stack = GROW_ARRAY(Value, vm.stack,
                              oldCapacity, vm.stackCapacity);
    }

    vm.stack[vm.stackCount] = value;
    vm.stackCount++;
}

// commented out for Ch. 15.3
// // move the stack pointer backwards to get to the most recently used slot in the array
// // look up the value at that index and return it
// // don’t need to remove the value, moving stackTop down marks that slot as no longer in use
// Value pop() {
//     vm.stackTop--;
//     return *vm.stackTop;
// }

// added for Ch. 15.3
Value pop() {
    vm.stackCount--;
    return vm.stack[vm.stackCount];
}

// when the interpreter executes a user’s program,
// it will spend ~90% of its time inside this function run()
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
    // reads the next byte from the bytecode, treats the resulting number as an index,
    // and looks up the corresponding Value in the chunk’s constant table
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op) \
do { \
double b = pop(); \
double a = pop(); \
push(a op b); \
} while (false)

    for (;;) {
        // commented out for Ch. 15.3
//         // when this flag is defined, the VM disassembles
//         // and prints each instruction right before executing it
// #ifdef DEBUG_TRACE_EXECUTION
//         // when tracing execution, show the current contents of the stack before interpreting the instruction
//         printf("          ");
//         // start at the first (bottom of the stack) and end at the top
//         for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
//             printf("[ ");
//             printValue(*slot);
//             printf(" ]");
//         }
//         printf("\n");
//
//         disassembleInstruction(vm.chunk,
//                                (int)(vm.ip - vm.chunk->code));
// #endif
        // added for Ch. 15.3
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");

        for (Value *slot = vm.stack; slot < vm.stack + vm.stackCount; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }

        printf("\n");

        disassembleInstruction(vm.chunk,
                               (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;
            case OP_NEGATE:   push(-pop()); break;
            case OP_RETURN: {
                printValue(pop());
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

// to make scoping more explicit, macro definitions are confined to their function
// define the macros at the beginning and undefine them at the end
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

// have a compiler that reports static errors and
// a VM that detects runtime errors so
// the interpreter will use this to know how to set the exit code of the process
InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    // run() is the function that actually runs the bytecode instructions
    return run();
}