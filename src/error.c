#include "error.h"
#include <stdio.h>

static int error_count = 0;

void error(const char *filename, int line, int col, const char *fmt, ...) {
  va_list args;
  printf("%s:%d:%d: error: ", filename, line, col);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
  error_count++;
}

int get_error_count(void) {
  return error_count;
}
