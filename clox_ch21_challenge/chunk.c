#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

// to initialize a new chunk
void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;

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
}

// to append a byte to the end of the chunk
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->capacity < chunk->count + 1) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(oldCapacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code,
            oldCapacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines,
            oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    // store the line number in the array
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

// to add a new constant to the chunk
int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}