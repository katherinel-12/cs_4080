#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

// to initialize a new chunk
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

    // added for Challenge 14.1
    chunk->lineCount = 0;
    chunk->lineCapacity = 0;

    // every time we touch the code array, make a corresponding change to the line number array
    // starting with initialization
    chunk->lines = NULL;

    // initialize a new chunk and initialize its constant list
    initValueArray(&chunk->constants);
}

// to free the memory of the chunk
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);

    FREE_ARRAY(int, chunk->lines, chunk->capacity);

    // free the constants when we free the chunk
    freeValueArray(&chunk->constants);
    initChunk(chunk);

    // added for Challenge 14.1
    FREE_ARRAY(LineStart, chunk->lines, chunk->lineCapacity);
}

// commented out for Challenge 14.1
// // to append a byte to the end of the chunk
// void writeChunk(Chunk* chunk, uint8_t byte, int line) {
//     if (chunk->capacity < chunk->count + 1) {
//         int oldCapacity = chunk->capacity;
//         chunk->capacity = GROW_CAPACITY(oldCapacity);
//         chunk->code = GROW_ARRAY(uint8_t, chunk->code,
//             oldCapacity, chunk->capacity);
//         chunk->lines = GROW_ARRAY(int, chunk->lines,
//             oldCapacity, chunk->capacity);
//     }
//
//     chunk->code[chunk->count] = byte;
//     // store the line number in the array
//     chunk->lines[chunk->count] = line;
//     chunk->count++;
// }

// added for Challenge 14.1
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
            oldCapacity, chunk->capacity);
        // Don't grow line array here...
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;

    // See if we're still on the same line.
    if (chunk->lineCount > 0 &&
        chunk->lines[chunk->lineCount - 1].line == line) {
        return;
        }

    // Append a new LineStart.
    if (chunk->lineCapacity < chunk->lineCount + 1) {
        int oldCapacity = chunk->lineCapacity;
        chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
        chunk->lines = GROW_ARRAY(LineStart, chunk->lines,
                                  oldCapacity, chunk->lineCapacity);
    }

    LineStart* lineStart = &chunk->lines[chunk->lineCount++];
    lineStart->offset = chunk->count - 1;
    lineStart->line = line;
}

// added for Challenge 14.2
void writeConstant(Chunk* chunk, Value value, int line) {
    int index = addConstant(chunk, value);
    if (index < 256) {
        writeChunk(chunk, OP_CONSTANT, line);
        writeChunk(chunk, (uint8_t)index, line);
    } else {
        writeChunk(chunk, OP_CONSTANT_LONG, line);
        writeChunk(chunk, (uint8_t)(index & 0xff), line);
        writeChunk(chunk, (uint8_t)((index >> 8) & 0xff), line);
        writeChunk(chunk, (uint8_t)((index >> 16) & 0xff), line);
    }
}

// to add a new constant to the chunk
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}

// added for Challenge 14.1
int getLine(Chunk* chunk, int instruction) {
    int start = 0;
    int end = chunk->lineCount - 1;

    for (;;) {
        int mid = (start + end) / 2;
        LineStart* line = &chunk->lines[mid];
        if (instruction < line->offset) {
            end = mid - 1;
        } else if (mid == chunk->lineCount - 1 ||
            instruction < chunk->lines[mid + 1].offset) {
            return line->line;
            } else {
                start = mid + 1;
            }
    }
}