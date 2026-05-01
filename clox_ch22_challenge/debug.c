#include <stdio.h>

#include "debug.h"
#include "value.h"

void disassembleChunk(Chunk* chunk, const char* name) {
    // print a header so we can tell which chunk we’re looking at
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char* name, Chunk* chunk,
                               int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int simpleInstruction(const char* name, int offset) {
    // print the name of the opcode
    printf("%s\n", name);
    return offset + 1;
}

static int byteInstruction(const char* name, Chunk* chunk,
                           int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int shortInstruction(const char* name, Chunk* chunk, int offset) {
    uint16_t slot = (chunk->code[offset + 1] << 8) |
                    chunk->code[offset + 2];
    printf("%-16s %4d\n", name, slot);
    return offset + 3;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    // prints the byte offset of the given instruction (where in the chunk this instruction is)
    printf("%04d ", offset);

    // show which source line each instruction was compiled from
    if (offset > 0 &&
    chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    // reads a single byte from the bytecode at the given offset (the opcode)
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        // for each kind of instruction, dispatch to a utility function for displaying it
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_LOCAL_LONG:
            return shortInstruction("OP_GET_LOCAL_LONG", chunk, offset); // Changed from byteInstruction
        case OP_SET_LOCAL_LONG:
            return shortInstruction("OP_SET_LOCAL_LONG", chunk, offset); // Changed from byteInstruction
        case OP_CONSTANT_LONG:
            return shortInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk,
                                       offset);
        case OP_DEFINE_FINAL_GLOBAL:
            return constantInstruction("OP_DEFINE_FINAL_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            // print the bug in our compiler
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

