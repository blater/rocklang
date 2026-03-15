#include "src/generation/asm_interop.h"

void main() {
  byte result;
  poke(to_word(1000), to_byte(42));
  result = peek(to_word(1000));
  
  if (result == 42) {
    // Success
  }
}
