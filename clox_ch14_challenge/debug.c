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

// added for Challenge 14.2
static int longConstantInstruction(const char* name, Chunk* chunk,
                                   int offset) {
    uint32_t constant = chunk->code[offset + 1] |
                       (chunk->code[offset + 2] << 8) |
                       (chunk->code[offset + 3] << 16);
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

static int simpleInstruction(const char* name, int offset) {
    // print the name of the opcode
    printf("%s\n", name);
    return offset + 1;
}

int disassembleInstruction(Chunk* chunk, int offset) {
    // prints the byte offset of the given instruction (where in the chunk this instruction is)
    printf("%04d ", offset);

    // commented out for Challenge 14.1
    // // show which source line each instruction was compiled from
    // if (offset > 0 &&
    // chunk->lines[offset] == chunk->lines[offset - 1]) {
    //     printf("   | ");
    // } else {
    //     printf("%4d ", chunk->lines[offset]);
    // }

    // added for Challenge 14.1
    int line = getLine(chunk, offset);
    if (offset > 0 && line == getLine(chunk, offset - 1)) {
        printf("   | ");
    } else {
        printf("%4d ", line);
    }

    // reads a single byte from the bytecode at the given offset (the opcode)
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        // for each kind of instruction, dispatch to a utility function for displaying it
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        // added for Challenge 14.2
        case OP_CONSTANT_LONG:
            return longConstantInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        default:
            // print the bug in our compiler
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

