#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE

// flag for diagnostic logging
#define DEBUG_TRACE_EXECUTION

// maximum capacity for local variables (256)
// the VM only supports up to 256 local variables in scope at one time
#define UINT8_COUNT (UINT8_MAX + 1)

#endif //CLOX_COMMON_H