// #include <stdio.h>
// #include <stdarg.h> // to use va_list and va_end
// #include <string.h>
// #include <time.h>
//
// #include "common.h"
// #include "vm.h"
// #include "debug.h"
// #include "compiler.h"
// #include "object.h"
// #include "memory.h"
//
// VM vm;
//
// static Value clockNative(int argCount, Value* args) {
//     return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
// }
//
// static void resetStack() {
//     // stackTop points to the beginning of the array to indicate the stack is empty
//     vm.stackTop = vm.stack;
//     vm.frameCount = 0;
// }
//
// // report the runtime error
// static void runtimeError(const char* format, ...) {
//     va_list args;
//     va_start(args, format);
//     vfprintf(stderr, format, args);
//     va_end(args);
//     fputs("\n", stderr);
//
//     // stack trace: to aid debugging runtime failures
//     // a print out of each function that was still executing when the program died, and where the execution was at the point that it died
//     // CallFrame* frame = &vm.frames[vm.frameCount - 1];
//     // size_t instruction = frame->ip - frame->function->chunk.code - 1;
//     // int line = frame->function->chunk.lines[instruction];
//     // fprintf(stderr, "[line %d] in script\n", line);
//     for (int i = vm.frameCount - 1; i >= 0; i--) {
//         CallFrame* frame = &vm.frames[i];
//         ObjFunction* function = frame->function;
//         size_t instruction = frame->ip - function->chunk.code - 1;
//         fprintf(stderr, "[line %d] in ",
//                 function->chunk.lines[instruction]);
//         if (function->name == NULL) {
//             fprintf(stderr, "script\n");
//         } else {
//             fprintf(stderr, "%s()\n", function->name->chars);
//         }
//     }
//
//     resetStack();
// }
//
// static void defineNative(const char* name, NativeFn function) {
//     push(OBJ_VAL(copyString(name, (int)strlen(name))));
//     push(OBJ_VAL(newNative(function)));
//     tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
//     pop();
//     pop();
// }
//
// // to create a VM
// void initVM() {
//     resetStack();
//     vm.objects = NULL;
//
//     initTable(&vm.globals);
//     initTable(&vm.strings); // table holding all strings is currently empty
//
//     defineNative("clock", clockNative);
// }
//
// // to tear down a VM
// void freeVM() {
//     freeTable(&vm.globals);
//     freeTable(&vm.strings); // clean up any resources used by the table holding strings
//     // free every object when the program is done
//     freeObjects();
// }
//
// // push a new value onto the top of the stack
// void push(Value value) {
//     // stores value in the array element at the top of the stack
//     *vm.stackTop = value;
//     // increment the pointer to point to the next unused slot in the array
//     vm.stackTop++;
// }
//
// // move the stack pointer backwards to get to the most recently used slot in the array
// // look up the value at that index and return it
// // don’t need to remove the value, moving stackTop down marks that slot as no longer in use
// Value pop() {
//     vm.stackTop--;
//     return *vm.stackTop;
// }
//
// // returns a Value from the stack but does not pop it
// // "distance" is how far down from the top of the stack to look: 0 is the top, 1 is one slot down, ...
// static Value peek(int distance) {
//     return vm.stackTop[-1 - distance];
// }
//
// static bool call(ObjFunction* function, int argCount) {
//     // a user could pass too many or too few arguments -? runtime error
//     if (argCount != function->arity) {
//         runtimeError("Expected %d arguments but got %d.",
//             function->arity, argCount);
//         return false;
//     }
//
//     // CallFrame array has a fixed size
//     // this is to ensure a deep call chain doesn’t overflow the CallFrame array
//     if (vm.frameCount == FRAMES_MAX) {
//         runtimeError("Stack overflow.");
//         return false;
//     }
//
//     CallFrame* frame = &vm.frames[vm.frameCount++];
//     frame->function = function;
//     frame->ip = function->chunk.code;
//     frame->slots = vm.stackTop - argCount - 1;
//     return true;
// }
//
// static bool callValue(Value callee, int argCount) {
//     if (IS_OBJ(callee)) {
//         switch (OBJ_TYPE(callee)) {
//             case OBJ_NATIVE: {
//                 NativeFn native = AS_NATIVE(callee);
//                 Value result = native(argCount, vm.stackTop - argCount);
//                 vm.stackTop -= argCount + 1;
//                 push(result);
//                 return true;
//             }
//             case OBJ_FUNCTION:
//                 return call(AS_FUNCTION(callee), argCount);
//             default:
//                 break; // Non-callable object type
//         }
//     }
//     runtimeError("Can only call functions and classes.");
//     return false;
// }
//
// // nil and false are falsey and every other value behaves like true
// static bool isFalsey(Value value) {
//     return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
// }
//
// // calculate the length of the result string based on the lengths of the operands
// // allocate a character array for the result and then copy the two halves in
// static void concatenate() {
//     ObjString* b = AS_STRING(pop());
//     ObjString* a = AS_STRING(pop());
//
//     int length = a->length + b->length;
//     char* chars = ALLOCATE(char, length + 1);
//     memcpy(chars, a->chars, a->length);
//     memcpy(chars + a->length, b->chars, b->length);
//     chars[length] = '\0';
//
//     ObjString* result = takeString(chars, length);
//     push(OBJ_VAL(result));
// }
//
// // when the interpreter executes a user’s program,
// // it will spend ~90% of its time inside this function run()
// static InterpretResult run() {
//     // store the current topmost CallFrame in a local variable inside the main bytecode execution function
//     CallFrame* frame = &vm.frames[vm.frameCount - 1];
//
// // replace the bytecode access macros with versions that access ip through that variable ( READ_BYTE(), READ_SHORT(), and READ_CONSTANT() )
// #define READ_BYTE() (*frame->ip++)
// #define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
// #define READ_SHORT() \
// (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
// #define READ_STRING() AS_STRING(READ_CONSTANT())
// // currently support: +, -, *, /
// // check that the two operands are both numbers
// // if fine, pop both operands and unwrap
// // apply the operator, wrap the result, and push it back on the stack
// #define BINARY_OP(valueType, op) \
// do { \
// if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
// runtimeError("Operands must be numbers."); \
// return INTERPRET_RUNTIME_ERROR; \
// } \
// double b = AS_NUMBER(pop()); \
// double a = AS_NUMBER(pop()); \
// push(valueType(a op b)); \
// } while (false)
//
//     for (;;) {
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
//         disassembleInstruction(&frame->function->chunk,
//         (int)(frame->ip - frame->function->chunk.code));
//
// #endif
//         uint8_t instruction;
//         switch (instruction = READ_BYTE()) {
//             case OP_CONSTANT: {
//                 Value constant = READ_CONSTANT();
//                 push(constant);
//                 break;
//             }
//             //  summons the appropriate value and pushes it onto the stack
//             case OP_NIL: push(NIL_VAL); break;
//             case OP_TRUE: push(BOOL_VAL(true)); break;
//             case OP_FALSE: push(BOOL_VAL(false)); break;
//             case OP_POP: pop(); break;
//             case OP_GET_LOCAL: {
//                 uint8_t slot = READ_BYTE();
//                 // accesses the current frame’s slots array (it accesses the given numbered slot relative to the beginning of that frame)
//                 push(frame->slots[slot]);
//                 break;
//             }
//             case OP_SET_LOCAL: {
//                 uint8_t slot = READ_BYTE();
//                 frame->slots[slot] = peek(0);
//                 break;
//             }
//             case OP_GET_GLOBAL: {
//                 ObjString* name = READ_STRING();
//                 Value value;
//                 if (!tableGet(&vm.globals, name, &value)) {
//                     runtimeError("Undefined variable '%s'.", name->chars);
//                     return INTERPRET_RUNTIME_ERROR;
//                 }
//                 push(value);
//                 break;
//             }
//             case OP_DEFINE_GLOBAL: {
//                 ObjString* name = READ_STRING();
//                 tableSet(&vm.globals, name, peek(0));
//                 pop();
//                 break;
//             }
//             case OP_SET_GLOBAL: {
//                 ObjString* name = READ_STRING();
//                 if (tableSet(&vm.globals, name, peek(0))) {
//                     tableDelete(&vm.globals, name);
//                     runtimeError("Undefined variable '%s'.", name->chars);
//                     return INTERPRET_RUNTIME_ERROR;
//                 }
//                 break;
//             }
//             case OP_EQUAL: {
//                 Value b = pop();
//                 Value a = pop();
//                 push(BOOL_VAL(valuesEqual(a, b)));
//                 break;
//             }
//             case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
//             case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
//             case OP_ADD: {
//                 // If both operands are strings, concatenates. If both numbers, adds
//                 if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
//                     concatenate();
//                 } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
//                     double b = AS_NUMBER(pop());
//                     double a = AS_NUMBER(pop());
//                     push(NUMBER_VAL(a + b));
//                 } else {
//                     runtimeError(
//                         "Operands must be two numbers or two strings.");
//                     return INTERPRET_RUNTIME_ERROR;
//                 }
//                 break;
//             }
//             case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
//             case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
//             case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
//             case OP_NOT:
//                 push(BOOL_VAL(isFalsey(pop())));
//                 break;
//             // before performing an op that requires a certain type, make sure the Value is of that type
//             case OP_NEGATE:
//                 // check to see if the Value on top of the stack is a number
//                 // if not, report the runtime error and stop the interpreter
//                 if (!IS_NUMBER(peek(0))) {
//                     runtimeError("Operand must be a number.");
//                     return INTERPRET_RUNTIME_ERROR;
//                 }
//                 push(NUMBER_VAL(-AS_NUMBER(pop())));
//                 break;
//             case OP_PRINT: {
//                 printValue(pop());
//                 printf("\n");
//                 break;
//             }
//             case OP_JUMP: {
//                 uint16_t offset = READ_SHORT();
//                 frame->ip += offset;
//                 break;
//             }
//             case OP_JUMP_IF_FALSE: {
//                 uint16_t offset = READ_SHORT();
//                 if (isFalsey(peek(0))) frame->ip += offset;
//                 break;
//             }
//             case OP_LOOP: {
//                 uint16_t offset = READ_SHORT();
//                 frame->ip -= offset;
//                 break;
//             }
//             case OP_CALL: {
//                 int argCount = READ_BYTE();
//                 if (!callValue(peek(argCount), argCount)) {
//                     return INTERPRET_RUNTIME_ERROR;
//                 }
//                 frame = &vm.frames[vm.frameCount - 1];
//                 break;
//             }
//             // case OP_RETURN: {
//             //     Value result = pop();
//             //     vm.frameCount--;
//             //     if (vm.frameCount == 0) {
//             //         pop();
//             //         return INTERPRET_OK;
//             //     }
//             //
//             //     vm.stackTop = frame->slots;
//             //     push(result);
//             //     frame = &vm.frames[vm.frameCount - 1];
//             //     break;
//             // }
//             case OP_RETURN: {
//                 Value result = pop();
//
//                 CallFrame* finishedFrame = frame;
//
//                 vm.frameCount--;
//                 if (vm.frameCount == 0) {
//                     pop();
//                     return INTERPRET_OK;
//                 }
//
//                 vm.stackTop = finishedFrame->slots;  // ← use saved pointer
//                 push(result);
//
//                 frame = &vm.frames[vm.frameCount - 1];
//                 break;
//             }
//         }
//     }
//
// // to make scoping more explicit, macro definitions are confined to their function
// // define the macros at the beginning and undefine them at the end
// #undef READ_BYTE
// #undef READ_SHORT
// #undef READ_CONSTANT
// #undef READ_STRING
// #undef BINARY_OP
// }
//
// /*
//  * pass the source code to the compiler
//  * compiler returns us a new ObjFunction containing the compiled top-level code
//  * if returns NULL, there was a compile-time error which the compiler has reported
//  *
//  * otherwise, store the function on the stack and prepare an initial CallFrame to execute its code
//  * stack slot 0 stores the function being called
//  *
//  * point to the function
//  *      initialize its ip to point to the beginning of the function’s bytecode
//  *      set up its stack window to start at the very bottom of the VM’s value stack
//  *
//  * the interpreter is ready to start executing code
//  */
// InterpretResult interpret(const char* source) {
//     ObjFunction* function = compile(source);
//     if (function == NULL) return INTERPRET_COMPILE_ERROR;
//
//     push(OBJ_VAL(function));
//     call(function, 0);
//
//     return run();
// }

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"

VM vm;

static NativeResult clockNative(int argCount, Value* args) {
    return (NativeResult){NUMBER_VAL((double)clock() / CLOCKS_PER_SEC), false};
    // return (NativeResult){NIL_VAL, true};
}

#include <math.h>

// power function
static NativeResult powNative(int argCount, Value* args) {
    if (!IS_NUMBER(args[0]) || !IS_NUMBER(args[1])) {
        return (NativeResult){NIL_VAL, true};
    }
    double result = pow(AS_NUMBER(args[0]), AS_NUMBER(args[1]));
    return (NativeResult){NUMBER_VAL(result), false};
}

static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();
}

// changed for ch 24.2 challenge
static void defineNative(const char* name, NativeFn function, int arity) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function, arity))); // Pass arity here
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM() {
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);
    defineNative("clock", clockNative, 0);
    defineNative("pow", powNative, 2);
}

void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) {
    return vm.stackTop[-1 - distance];
}

static bool call(ObjFunction* function, int argCount) {
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    }

    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame* frame = &vm.frames[vm.frameCount++];
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_NATIVE: {
                // ObjNative* native = AS_NATIVE_OBJ(callee); // Get the full object
                // if (argCount != native->arity) {
                //     runtimeError("Expected %d arguments but got %d.",
                //                  native->arity, argCount);
                //     return false;
                // }
                // NativeFn function = native->function;
                // Value result = function(argCount, vm.stackTop - argCount);
                // vm.stackTop -= argCount + 1;
                // push(result);
                // return true;
                ObjNative* native = AS_NATIVE_OBJ(callee);
                if (argCount != native->arity) {
                    runtimeError("Expected %d arguments but got %d.", native->arity, argCount);
                    return false;
                }

                NativeResult result = native->function(argCount, vm.stackTop - argCount);

                if (result.hasError) {
                    runtimeError("Native function error.");
                    return false;
                }

                vm.stackTop -= argCount + 1;
                push(result.value);
                return true;
            }
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            default:
                break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

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

static InterpretResult run() {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

    // added for ch 24.1 challenge
    register uint8_t* ip = frame->ip;

// #define READ_BYTE() (*frame->ip++)
// #define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
// #define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])

// changed for ch 24.1 challenge
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
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
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        disassembleInstruction(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_NIL:      push(NIL_VAL); break;
            case OP_TRUE:     push(BOOL_VAL(true)); break;
            case OP_FALSE:    push(BOOL_VAL(false)); break;
            case OP_POP:      pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]); // Frame-relative indexing
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
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
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError("Operands must be numbers or strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT:      push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_NEGATE:
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
                // frame->ip += offset;
                ip += offset;
                break;
            }
            // changed for ch 24.1 challenge
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0))) ip += offset; // Use local ip
                break;
            }
            // changed for ch 24.1 challenge
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset; // Use local ip
                break;
            }
            // changed for ch 24.1 challenge
            case OP_CALL: {
                int argCount = READ_BYTE();
                frame->ip = ip;

                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }

                frame = &vm.frames[vm.frameCount - 1];
                ip = frame->ip;
                break;
            }
           // changed for ch 24.1 challenge
            case OP_RETURN: {
                // Value result = pop();
                // vm.frameCount--;
                // if (vm.frameCount == 0) {
                //     pop();
                //     return INTERPRET_OK;
                // }
                //
                // vm.stackTop = frame->slots;
                // push(result);
                //
                // frame = &vm.frames[vm.frameCount - 1];
                // ip = frame->ip;
                // break;
                Value result = pop();
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }

                // 1. Reset the stack to the beginning of the frame we are leaving
                vm.stackTop = frame->slots;

                // 2. Push the result onto the caller's stack top
                push(result);

                // 3. IMPORTANT: Update our local 'frame' to the caller's frame
                frame = &vm.frames[vm.frameCount - 1];

                // 4. Update our local 'ip' to the caller's ip
                ip = frame->ip;
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
    ObjFunction* function = compile(source);
    if (function == NULL) return INTERPRET_COMPILE_ERROR;

    push(OBJ_VAL(function));
    call(function, 0);

    return run();
}