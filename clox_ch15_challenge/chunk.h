#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H
// a module to define the code representation
// using “chunk” to refer to sequences of bytecode

#include "common.h"
#include "value.h"

// each instruction has a one-byte operation code
// this number controls what kind of instruction we’re dealing with:
// add, subtract, look up variable, etc.
typedef enum {
    // produces a particular constant
    OP_CONSTANT,
    // other binary operators
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    // negates a value
    OP_NEGATE,
    // return from the current function
    OP_RETURN,
  } OpCode;

// the struct to hold all the other data along with the instructions
typedef struct {
    int count; // how many of those allocated entries are actually in use
    int capacity; // the number of elements in the array we have allocated
    /*
     * if the count is less than the capacity,
     * then there is already available space in the array
     *
     * if the count is equal to the capacity
     * allocate a new array with more capacity
     * copy the existing elements from the old array to the new one
     * store the new capacity
     * delete the old array
     * update code to point to the new array
     * store the element in the new array now that there is room
     * update the count
     */

    uint8_t* code;

    // array of ints where each index stores the source line number
    // corresponding to the instruction at the same offset in the bytecode
    int* lines;

    // in value.c there is a growable arrays of values
    // this is to store the chunk’s constants
    ValueArray constants;

} Chunk;

// to initialize a new chunk
void initChunk(Chunk* chunk);

// to free the memory of the chunk
void freeChunk(Chunk* chunk);

// to append a byte to the end of the chunk
void writeChunk(Chunk* chunk, uint8_t byte, int line);

// to add a new constant to the chunk
int addConstant(Chunk* chunk, Value value);

#endif //CLOX_CHUNK_H