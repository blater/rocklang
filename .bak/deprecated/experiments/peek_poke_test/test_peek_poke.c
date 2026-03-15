#include <stdio.h>
#include <stdint.h>

/* External Z80 assembly functions */
extern uint8_t peek(uint16_t addr);
extern uint16_t poke(uint16_t addr, uint8_t val);

int main(void) {

  printf("=== Debug Peek/Poke Test ===\n\n");

  uint16_t addr = poke(16385, 99);
  printf("poke addr: %d\n", addr);

  uint8_t result = peek(16385);


  if (result == 99) {
    printf("PASS\n");
  } else {
    printf("FAIL (expected 99, got %d)\n", result);
  }

  printf("\nPress any key to exit...\n");
  getchar();

  return 0;
}
