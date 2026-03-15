#include <stdio.h>

extern struct _FILE *_stdout;
extern struct _FILE *__stdio_open_file_list;

struct _FILE { char x; };
struct _FILE *_stdout = 0;
struct _FILE *__stdio_open_file_list = 0;

int main(void) {
  printf("Hello World\n");
  return 0;
}
