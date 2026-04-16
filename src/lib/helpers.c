#include "helpers.h"

byte odd(int x)  { return (byte)(x & 1); }
byte even(int x) { return (byte)(((x & 1) == 0) ? 1 : 0); }

byte hi(word w) { return (byte)((w >> 8) & 0xFF); }
byte lo(word w) { return (byte)(w & 0xFF); }

word swap(word w) {
  return (word)(((w & 0xFF) << 8) | ((w >> 8) & 0xFF));
}

char upcase(char c) {
  if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
  return c;
}

char locase(char c) {
  if (c >= 'A' && c <= 'Z') return (char)(c + ('a' - 'A'));
  return c;
}

int abs_int(int x) { return x < 0 ? -x : x; }
word abs_word(word w) { return w; }
