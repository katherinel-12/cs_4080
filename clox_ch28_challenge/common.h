#ifndef CLOX_COMMON_H
#define CLOX_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE

// flag for diagnostic logging
// #define DEBUG_TRACE_EXECUTION

// the GC runs as often as possible
// this means it is bad for performance but
// good for flushing out memory management bugs
#define DEBUG_STRESS_GC

// clox prints information to the console when
// it does something with dynamic memory
#define DEBUG_LOG_GC

// maximum capacity for local variables (256)
// the VM only supports up to 256 local variables in scope at one time
#define UINT8_COUNT (UINT8_MAX + 1)

#endif //CLOX_COMMON_H