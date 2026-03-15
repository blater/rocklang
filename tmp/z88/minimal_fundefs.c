#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char *data;
  int length;
} string;

void print(string s) {
  int i;
  if (s.data == NULL) {
    printf("NULL");
    fflush(stdout);
    return;
  }
  for (i = 0; i < s.length; i = i + 1) {
    putchar(s.data[i]);
  }
  fflush(stdout);
}

string __rock_make_string(const char *data, size_t length) {
  string s;
  s.data = (char *)data;
  s.length = (int)length;
  return s;
}

string new_string(string s) {
  return s;
}

string __to_string_int(int n) {
  return __rock_make_string("0", 1);
}

void poke(int addr, unsigned char val) {
  unsigned char *ptr = (unsigned char *)addr;
  *ptr = val;
}

unsigned char peek(int addr) {
  unsigned char *ptr = (unsigned char *)addr;
  return *ptr;
}

void fill_cmd_args(int argc, char **argv) {
}

/* Stub stdio symbols for z88dk */
struct _FILE { char x; };
struct _FILE *_stdout = 0;
struct _FILE *__stdio_open_file_list = 0;
