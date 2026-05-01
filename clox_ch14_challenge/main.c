#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include <assert.h>
#include <stdio.h>
#include "memory_hardcore.h"

int main(int argc, const char* argv[]) {
    // Chunk chunk;
    // initChunk(&chunk);
    //
    // int constant = addConstant(&chunk, 1.2);
    // writeChunk(&chunk, OP_CONSTANT, 123);
    // writeChunk(&chunk, constant, 123);
    //
    // writeChunk(&chunk, OP_RETURN, 123);
    //
    // // disassembler is given binary machine code
    // // it spits out human-readable text of the instructions
    // disassembleChunk(&chunk, "test chunk");
    // freeChunk(&chunk);
    //
    // return 0;

    // Chunk chunk;
    // initChunk(&chunk);
    //
    // // Simulate writing instructions with line numbers
    // writeChunk(&chunk, 0x01, 1);
    // writeChunk(&chunk, 0x02, 1);
    // writeChunk(&chunk, 0x03, 1);
    //
    // writeChunk(&chunk, 0x04, 2);
    // writeChunk(&chunk, 0x05, 2);
    //
    // writeChunk(&chunk, 0x06, 3);
    //
    // // Print raw LineStart table
    // printf("LineStarts:\n");
    // for (int i = 0; i < chunk.lineCount; i++) {
    //     printf("  [%d] offset=%d, line=%d\n",
    //            i,
    //            chunk.lines[i].offset,
    //            chunk.lines[i].line);
    // }
    //
    // // Test getLine()
    // printf("\nInstruction → Line mapping:\n");
    // for (int i = 0; i < chunk.count; i++) {
    //     printf("  offset %d → line %d\n",
    //            i,
    //            getLine(&chunk, i));
    // }
    //
    // freeChunk(&chunk);
    // return 0;

    // Chunk chunk;
    // initChunk(&chunk);
    //
    // printf("=== Test 1: Small number of constants ===\n");
    // for (int i = 0; i < 10; i++) {
    //     writeConstant(&chunk, (double)i, 1);
    // }
    // disassembleChunk(&chunk, "Small Constants");
    //
    // freeChunk(&chunk);
    //
    // // ----------------------------------------
    //
    // initChunk(&chunk);
    //
    // printf("\n=== Test 2: Crossing 256 boundary ===\n");
    // for (int i = 0; i < 260; i++) {
    //     writeConstant(&chunk, (double)i, 1);
    // }
    // disassembleChunk(&chunk, "Boundary Test");
    //
    // freeChunk(&chunk);
    //
    // // ----------------------------------------
    //
    // initChunk(&chunk);
    //
    // printf("\n=== Test 3: Exact boundary values ===\n");
    //
    // // Fill up to index 255
    // for (int i = 0; i < 256; i++) {
    //     writeConstant(&chunk, (double)i, 1);
    // }
    //
    // // Critical boundary checks
    // writeConstant(&chunk, 256.0, 1);
    // writeConstant(&chunk, 257.0, 1);
    //
    // disassembleChunk(&chunk, "Exact Boundary Test");
    //
    // freeChunk(&chunk);
    //
    // // ----------------------------------------
    //
    // initChunk(&chunk);
    //
    // printf("\n=== Test 4: Raw byte inspection ===\n");
    //
    // for (int i = 250; i < 265; i++) {
    //     writeConstant(&chunk, (double)i, 1);
    // }
    //
    // printf("Raw bytecode:\n");
    // for (int i = 0; i < chunk.count; i++) {
    //     printf("%02X ", chunk.code[i]);
    // }
    // printf("\n");
    //
    // disassembleChunk(&chunk, "Byte Inspection");
    //
    // freeChunk(&chunk);
    //
    // return 0;
//



    assert(hardcore_allocator_init(256));
    printf("init ok (256 bytes)\n");

    int* a = (int*)hardcore_reallocate(NULL, 0, sizeof(int) * 4);
    assert(a != NULL);
    for (int i = 0; i < 4; i++) a[i] = i + 1;
    printf("alloc 4 ints at %p\n", (void*)a);

    int* b = (int*)hardcore_reallocate(a, sizeof(int) * 4, sizeof(int) * 8);
    assert(b != NULL);
    assert(b[0] == 1 && b[1] == 2 && b[2] == 3 && b[3] == 4);
    printf("grow to 8 ints at %p (copied old values)\n", (void*)b);

    void* freed = hardcore_reallocate(b, sizeof(int) * 8, 0);
    assert(freed == NULL);
    printf("free request returned NULL (no reclaim in bump allocator)\n");

    void* tooBig = hardcore_reallocate(NULL, 0, 10000);
    assert(tooBig == NULL);
    printf("oversized alloc returned NULL\n");

    hardcore_allocator_shutdown();
    printf("hardcore bump allocator demo passed\n");

    return 0;
}