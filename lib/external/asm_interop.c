#include "asm_interop.h"
#include <stdlib.h>
#include <string.h>

// Host target: Simulate memory with a static buffer
// ZXN target: Direct memory access via pointers
#ifndef __SDCC
#define SIMULATED_MEMORY_SIZE 65536
static byte simulated_memory[SIMULATED_MEMORY_SIZE] = {0};
#endif

byte peek(word address) {
#ifdef __SDCC
  // Z88DK (sccz80): Direct pointer dereference
  // Use volatile to prevent compiler optimizations
  return *(volatile byte*)address;
#else
  // Host target: Read from simulated memory buffer
  // word is 16-bit, so max value is 65535, and buffer is 65536 bytes
  // No bounds check needed - all word values are valid
  return simulated_memory[address];
#endif
}

void poke(word address, byte val) {
#ifdef __SDCC
  // Z88DK (sccz80): Direct pointer write
  // Use volatile to prevent compiler optimizations
  *(volatile byte*)address = val;
#else
  // Host target: Write to simulated memory buffer
  // word is 16-bit, so max value is 65535, and buffer is 65536 bytes
  // No bounds check needed - all word values are valid
  simulated_memory[address] = val;
#endif
}

// Note: fill_cmd_args is defined in fundefs_internal.c
// This stub is not needed since fundefs_internal.c is always linked for host targets

