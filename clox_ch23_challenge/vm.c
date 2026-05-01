#include <stdio.h>
#include <stdarg.h> // to use va_list and va_end
#include <string.h>

#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"

VM vm;

static void resetStack() {
    // stackTop points to the beginning of the array to indicate the stack is empty
    vm.stackTop = vm.stack;
}

// report the runtime error
static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

// to create a VM
void initVM() {
    resetStack();
    vm.objects = NULL;

    initTable(&vm.globals);
    initTable(&vm.strings); // table holding all strings is currently empty
}

// to tear down a VM
void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings); // clean up any resources used by the table holding strings
    // free every object when the program is done
    freeObjects();
}

// push a new value onto the top of the stack
void push(Value value) {
    // stores value in the array element at the top of the stack
    *vm.stackTop = value;
    // increment the pointer to point to the next unused slot in the array
    vm.stackTop++;
}

// move the stack pointer backwards to get to the most recently used slot in the array
// look up the value at that index and return it
// don’t need to remove the value, moving stackTop down marks that slot as no longer in use
Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

// returns a Value from the stack but does not pop it
// "distance" is how far down from the top of the stack to look: 0 is the top, 1 is one slot down, ...
static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

// nil and false are falsey and every other value behaves like true
static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

// calculate the length of the result string based on the lengths of the operands
// allocate a character array for the result and then copy the two halves in
static void concatenate() {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

// when the interpreter executes a user’s program,
// it will spend ~90% of its time inside this function run()
static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
    // reads the next byte from the bytecode, treats the resulting number as an index,
    // and looks up the corresponding Value in the chunk’s constant table
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_SHORT() \
(vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
// currently support: +, -, *, /
// check that the two operands are both numbers
// if fine, pop both operands and unwrap
// apply the operator, wrap the result, and push it back on the stack
#define BINARY_OP(valueType, op) \
do { \
if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
runtimeError("Operands must be numbers."); \
return INTERPRET_RUNTIME_ERROR; \
} \
double b = AS_NUMBER(pop()); \
double a = AS_NUMBER(pop()); \
push(valueType(a op b)); \
} while (false)

    for (;;) {
        // when this flag is defined, the VM disassembles
        // and prints each instruction right before executing it
#ifdef DEBUG_TRACE_EXECUTION
        // when tracing execution, show the current contents of the stack before interpreting the instruction
        printf("          ");
        // start at the first (bottom of the stack) and end at the top
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
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
            //  summons the appropriate value and pushes it onto the stack
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                // If both operands are strings, concatenates. If both numbers, adds
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                        "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:
                push(BOOL_VAL(isFalsey(pop())));
                break;
            // before performing an op that requires a certain type, make sure the Value is of that type
            case OP_NEGATE:
                // check to see if the Value on top of the stack is a number
                // if not, report the runtime error and stop the interpreter
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_PRINT: {
                printValue(pop());
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) vm.ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                vm.ip -= offset;
                break;
            }
            case OP_DUP: push(peek(0)); break;
            case OP_RETURN: {
                // printValue(pop());
                // printf("\n");
                // exit interpreter
                return INTERPRET_OK;
            }
        }
    }

// to make scoping more explicit, macro definitions are confined to their function
// define the macros at the beginning and undefine them at the end
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

/*
 * create a new empty chunk and pass it over to the compiler
 * the compiler takes the user’s program and fills up the chunk with bytecode
 * the completed bytecode chunk is sent to the VM to be executed
 * when the VM finishes, free the chunk and done
 * if there's an error, compile() returns false and discard the unusable chunk
 */
InterpretResult interpret(const char* source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}