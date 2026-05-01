#include <stdio.h>
#include <string.h>

#include "common.h"
#include "object.h"
#include "value.h"
#include "vm.h"

// Forward declaration of the internal VM function
// If concatenate is 'static' in vm.c, you may need to
// temporarily remove 'static' from its definition in vm.c
void concatenate();

void runStringTests() {
    printf("\n--- STARTING STRING TESTS ---\n");
    initVM();

    // Test 1: Memory Layout
    // We check if the flexible array member sits right after the struct
    const char* testSource = "Lox";
    ObjString* string = copyString(testSource, 3);

    size_t offset = (uint8_t*)string->chars - (uint8_t*)string;
    printf("Test 1 (Layout): ");
    if (offset == sizeof(ObjString)) {
        printf("PASS (Offset: %zu)\n", offset);
    } else {
        printf("FAIL (Expected %zu, got %zu)\n", sizeof(ObjString), offset);
    }

    // Test 2: Concatenation via VM Stack
    // We push two strings and let the VM logic merge them
    push(OBJ_VAL(copyString("Flexible ", 9)));
    push(OBJ_VAL(copyString("Arrays", 6)));

    printf("Test 2 (Stack Concat): ");
    concatenate();

    Value resultValue = pop();
    if (!IS_STRING(resultValue)) {
        printf("FAIL: Result is not a string type.\n");
    } else {
        ObjString* result = AS_STRING(resultValue);
        if (strcmp(result->chars, "Flexible Arrays") == 0) {
            printf("PASS (\"%s\")\n", result->chars);
        } else {
            printf("FAIL (Got \"%s\")\n", result->chars);
        }
    }

    // Test 3: Null Terminator Check
    printf("Test 3 (Null Terminator): ");
    ObjString* res = AS_STRING(resultValue);
    if (res->chars[res->length] == '\0') {
        printf("PASS (Found \\0 at index %d)\n", res->length);
    } else {
        printf("FAIL (Missing terminator!)\n");
    }

    printf("--- ALL TESTS COMPLETE ---\n\n");
    freeVM();
}