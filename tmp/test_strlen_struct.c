#include <string.h>

typedef struct {
  char *data;
  int length;
} mystring;

mystring make_string(char *d, int l) {
  mystring s;
  s.data = d;
  s.length = l;
  return s;
}

mystring cstr_to_string(char *cstr) {
  return make_string(cstr, strlen(cstr));
}

void main() {
  mystring s = cstr_to_string("test");
}
