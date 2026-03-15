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

void main() {
  mystring s = make_string("test", 4);
}
