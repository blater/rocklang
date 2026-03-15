#include "./src/generation/fundefs.h"
#include "./src/generation/fundefs_internal.h"
#include "./src/generation/typedefs.h"

/* Inlined allocator code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct alloc_elem_t {
  void *ptr;
  int scope;
} alloc_elem_t;

typedef struct alloc_stack_t {
  alloc_elem_t *data;
  void **persistents;
  size_t capacity;
  size_t capacity_p;
  int length;
  int length_p;
  size_t scope;
} alloc_stack_t;

#define INIT_CAP_ALLOC_STACK 1024

void init_stack(alloc_stack_t *alloc) {
  alloc->capacity = INIT_CAP_ALLOC_STACK;
  alloc->capacity_p = INIT_CAP_ALLOC_STACK;
  alloc->data = malloc(sizeof(alloc_elem_t) * INIT_CAP_ALLOC_STACK);
  alloc->persistents = malloc(sizeof(void *) * INIT_CAP_ALLOC_STACK);
  alloc->length = 0;
  alloc->length_p = 0;
}

void kill_stack(alloc_stack_t alloc) {
  for (int i = 0; i < alloc.length; i++)
    free(alloc.data[i].ptr);
  for (int i = 0; i < alloc.length_p; i++)
    free(alloc.persistents[i]);
  free(alloc.data);
  free(alloc.persistents);
}

void push_stack(alloc_stack_t *alloc, void *ptr) {
  if (alloc->length >= (int)alloc->capacity) {
    alloc->capacity = alloc->capacity * 2;
    alloc->data = realloc(alloc->data, alloc->capacity * sizeof(alloc_elem_t));
  }
  alloc_elem_t pushed;
  pushed.ptr = ptr;
  pushed.scope = alloc->scope;
  alloc->data[alloc->length++] = pushed;
}

void push_persistent(alloc_stack_t *alloc, void *ptr) {
  if (alloc->length_p >= (int)alloc->capacity_p) {
    alloc->capacity_p = alloc->capacity_p * 2;
    alloc->persistents =
        realloc(alloc->persistents, alloc->capacity_p * sizeof(void *));
  }
  alloc->persistents[alloc->length_p++] = ptr;
}

void *pop_stack(alloc_stack_t *alloc) {
  if (alloc->length == 0)
    return NULL;
  return alloc->data[--alloc->length].ptr;
}

size_t get_top_scope(alloc_stack_t alloc) {
  if (alloc.length <= 0)
    return -1;
  return alloc.data[alloc.length - 1].scope;
}

void new_scope(alloc_stack_t *alloc) { alloc->scope++; }

void end_scope(alloc_stack_t *alloc) {
  if (alloc->length == 0)
    return;
  while (get_top_scope(*alloc) >= alloc->scope && alloc->length > 0)
    free(pop_stack(alloc));
  if (alloc->scope >= 1)
    alloc->scope--;
}

void *allocate(alloc_stack_t *alloc, size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    printf("Could not allocate\n");
    exit(1);
  }
  push_stack(alloc, ptr);
  return ptr;
}

void *allocate_persistent(alloc_stack_t *alloc, size_t size) {
  void *ptr = malloc(size);
  if (ptr == NULL) {
    printf("Could not allocate persistent\n");
    exit(1);
  }
  memset(ptr, 0, size);
  push_persistent(alloc, ptr);
  return ptr;
}

void *reallocate(alloc_stack_t *alloc, void *ptr, size_t size) {
  void *res = realloc(ptr, size);
  if (res == NULL) {
    printf("Could not reallocate data !\n");
    exit(1);
  }
  for (int i = 0; i < alloc->length; i++) {
    void *test = alloc->data[i].ptr;
    if (test == ptr) {
      alloc->data[i].ptr = res;
      break;
    }
  }
  return res;
}

void *reallocate_persistent(alloc_stack_t *alloc, void *ptr, size_t size) {
  void *res = realloc(ptr, size);
  if (res == NULL) {
    printf("Could not reallocate data !\n");
    exit(1);
  }
  for (int i = 0; i < alloc->length_p; i++) {
    void *test = alloc->persistents[i];
    if (test == ptr) {
      alloc->persistents[i] = res;
      break;
    }
  }
  return res;
}

alloc_stack_t global;

void init_compiler_stack(void) { init_stack(&global); }

void kill_compiler_stack(void) { kill_stack(global); }

void new_compiler_scope(void) { new_scope(&global); }

void end_compiler_scope(void) { end_scope(&global); }

void *allocate_compiler(size_t size) { return allocate(&global, size); }

void *allocate_compiler_persistent(size_t size) {
  return allocate_persistent(&global, size);
}

void *reallocate_compiler(void *ptr, size_t size) {
  return reallocate(&global, ptr, size);
}

void *reallocate_compiler_persistent(void *ptr, size_t size) {
  return reallocate_persistent(&global, ptr, size);
}

typedef struct file *file;
typedef enum file_mode file_mode;
typedef enum term_color term_color;
file open_file(string filename, file_mode mode);

void close_file(file f);

string get_file_contents(file f);

void pfile(file f, string s);

void print_color(string src, term_color col);

void print_underline(string s);

void print_info();

void print_cmd();

void print_error();

void implemented(string s);

void compiler_assert(boolean b, string s);

void print_int(int n);

string string_of_int(int n);

string toString(int n);

string create_string(string src, int offset, int length);

string cons_str(string src, int offset);

__internal_dynamic_array_t file_make_array(void) {
  return __internal_make_array(sizeof(file), 0);
}

void file_push_array(__internal_dynamic_array_t arr, file elem) {
  __internal_push_array(arr, &elem);
}

file file_pop_array(__internal_dynamic_array_t arr) {
  file *res = __internal_pop_array(arr);
  return *res;
}

file file_get_elem(__internal_dynamic_array_t arr, size_t index) {
  file *res = __internal_get_elem(arr, index);
  if (res == NULL) {
    printf("NULL ELEMENT IN file_get_elem");
    exit(1);
  }
  return *res;
}

void file_set_elem(__internal_dynamic_array_t arr, size_t index, file elem) {
  __internal_set_elem(arr, index, &elem);
}

void file_insert(__internal_dynamic_array_t arr, size_t index, file elem) {
  __internal_insert(arr, index, &elem);
}

struct file {
  string Filename;
  string Data;
  int Mode;
};
enum file_mode { READ_MODE, WRITE_MODE, CLOSED };
file open_file(string filename, file_mode mode) {
  struct file tmp_res = {.Filename = filename,
                         .Data = new_string((string){.data = "", .length = 0}),
                         .Mode = mode};
  file res = allocate_compiler_persistent(sizeof(struct file));
  *res = tmp_res;
  if (mode == READ_MODE)
    res->Data = read_file(filename);
  else if (mode != WRITE_MODE) {
    print(new_string((string){.data = "Could not open file \'", .length = 21}));
    print(filename);
    print(new_string(
        (string){.data = "\': Invalid mode specified\n", .length = 26}));
    exit(1);
  }
  return res;
}

void close_file(file f) {
  if (f->Mode == WRITE_MODE)
    write_string_to_file(f->Data, f->Filename);
  f->Data = new_string((string){.data = "", .length = 0});
  f->Filename = new_string((string){.data = "", .length = 0});
  f->Mode = CLOSED;
}

string get_file_contents(file f) {
  if (f->Mode != READ_MODE) {
    print_error();
    print(new_string((string){.data = "Cannot access contents of a file that "
                                      "is not opened or not readable\n",
                              .length = 68}));
    exit(1);
  }
  return f->Data;
}

void pfile(file f, string s) { f->Data = concat(f->Data, s); }

enum term_color { ROCKER_WHITE, ROCKER_RED, ROCKER_GREEN, ROCKER_YELLOW };
void print_color(string src, term_color col) {
  if (col == ROCKER_WHITE)
    print(new_string((string){.data = "\e[1;37m", .length = 8}));
  else if (col == ROCKER_RED)
    print(new_string((string){.data = "\e[1;31m", .length = 8}));
  else if (col == ROCKER_GREEN)
    print(new_string((string){.data = "\e[1;42m", .length = 8}));
  else if (col == ROCKER_YELLOW)
    print(new_string((string){.data = "\e[1;43m", .length = 8}));
  print(src);
  print(new_string((string){.data = "\e[0m", .length = 5}));
}

void print_underline(string s) {
  print(new_string((string){.data = "\e[4m", .length = 5}));
  print(s);
  print(new_string((string){.data = "\e[0m", .length = 5}));
}

void print_info() {
  print(new_string((string){.data = "\e[1;32m", .length = 8}));
  print(new_string((string){.data = "[INFO] ", .length = 7}));
  print(new_string((string){.data = "\e[0m", .length = 5}));
}

void print_cmd() {
  print(new_string((string){.data = "\e[1;33m", .length = 8}));
  print(new_string((string){.data = "[CMD]  ", .length = 7}));
  print(new_string((string){.data = "\e[0m", .length = 5}));
}

void print_error() {
  print(new_string((string){.data = "\e[1;31m", .length = 8}));
  print(new_string((string){.data = "[ERROR] ", .length = 8}));
  print(new_string((string){.data = "\e[0m", .length = 5}));
}

void implemented(string s) {
  print(new_string((string){.data = "TODO: ", .length = 6}));
  print(s);
  print(
      new_string((string){.data = " is not yet implemented\n", .length = 24}));
  exit(1);
}

void compiler_assert(boolean b, string s) {
  if (b == 0) {
    print(new_string((string){.data = "Assertion failed: ", .length = 18}));
    print(s);
    exit(1);
  }
}

void print_int(int n) {
  int a = n / 10;
  if (a)
    print_int(a);
  putchar(n % 10 + '0');
}

string string_of_int(int n) {
  __internal_dynamic_array_t chars = __internal_make_array(sizeof(char), 0);
  if (n == 0)
    char_push_array(chars, '0');
  while (n > 0) {
    char_push_array(chars, n % 10 + '0');
    n = n / 10;
  };
  string res = new_string((string){.data = "", .length = 0});
  for (int i = 0; i <= (int)length(chars) - 1; i++)
    res = concat(res, char_get_elem(chars, length(chars) - 1 - i));
  ;
  return res;
}

string toString(int n) { return string_of_int(n); }

string create_string(string src, int offset, int length) {
  string res = new_string((string){.data = "", .length = 0});
  for (int i = 0; i <= (int)length - 1; i++) {
    char c = get_nth_char(src, offset + i);
    res = concat(res, c);
  };
  return res;
}

string cons_str(string src, int offset) {
  return create_string(src, offset, get_string_length(src) - offset);
}

int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    print(toString(42));
    print(new_string((string){.data = "\n", .length = 1}));
    int num = 12345;
    print(toString(num));
    print(new_string((string){.data = "\n", .length = 1}));
    print(toString(10 + 20));
    print(new_string((string){.data = "\n", .length = 1}));
    int x = 7;
    print(toString(x * 3));
    print(new_string((string){.data = "\n", .length = 1}));
  }
  kill_compiler_stack();
  return 0;
}
