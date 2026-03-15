#include "src/generation/typedefs.h"
#include "src/generation/asm_interop.h"
#include <stdio.h>

int main() {
  // Use safe RAM address (above program space)
  word test_addr = (word)0xE000;  // 57344 - high RAM, safe to write

  // Test 1: poke 42, peek back
  poke(test_addr, (byte)42);
  if (peek(test_addr) == 42) {
    printf("Test 1: PASS\n");
  } else {
    printf("Test 1: FAIL\n");
  }

  // Test 2: poke 255
  poke(test_addr, (byte)255);
  if (peek(test_addr) == 255) {
    printf("Test 2: PASS\n");
  } else {
    printf("Test 2: FAIL\n");
  }

  // Test 3: poke 0
  poke(test_addr, (byte)0);
  if (peek(test_addr) == 0) {
    printf("Test 3: PASS\n");
  } else {
    printf("Test 3: FAIL\n");
  }

  // Test 4: different address
  word addr2 = (word)0xE100;  // 57600
  poke(addr2, (byte)123);
  if (peek(addr2) == 123) {
    printf("Test 4: PASS\n");
  } else {
    printf("Test 4: FAIL\n");
  }

  printf("All done\n");
  return 0;
}
