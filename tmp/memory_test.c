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

#ifndef INIT_CAP_ALLOC_STACK
#define INIT_CAP_ALLOC_STACK 1024
#endif

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
typedef struct Person *Person;
typedef struct Team *Team;
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

__internal_dynamic_array_t Person_make_array(void) {
  return __internal_make_array(sizeof(Person), 0);
}

void Person_push_array(__internal_dynamic_array_t arr, Person elem) {
  __internal_push_array(arr, &elem);
}

Person Person_pop_array(__internal_dynamic_array_t arr) {
  Person *res = __internal_pop_array(arr);
  return *res;
}

Person Person_get_elem(__internal_dynamic_array_t arr, size_t index) {
  Person *res = __internal_get_elem(arr, index);
  if (res == NULL) {
    printf("NULL ELEMENT IN Person_get_elem");
    exit(1);
  }
  return *res;
}

void Person_set_elem(__internal_dynamic_array_t arr, size_t index,
                     Person elem) {
  __internal_set_elem(arr, index, &elem);
}

void Person_insert(__internal_dynamic_array_t arr, size_t index, Person elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t Team_make_array(void) {
  return __internal_make_array(sizeof(Team), 0);
}

void Team_push_array(__internal_dynamic_array_t arr, Team elem) {
  __internal_push_array(arr, &elem);
}

Team Team_pop_array(__internal_dynamic_array_t arr) {
  Team *res = __internal_pop_array(arr);
  return *res;
}

Team Team_get_elem(__internal_dynamic_array_t arr, size_t index) {
  Team *res = __internal_get_elem(arr, index);
  if (res == NULL) {
    printf("NULL ELEMENT IN Team_get_elem");
    exit(1);
  }
  return *res;
}

void Team_set_elem(__internal_dynamic_array_t arr, size_t index, Team elem) {
  __internal_set_elem(arr, index, &elem);
}

void Team_insert(__internal_dynamic_array_t arr, size_t index, Team elem) {
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
                         .Data = new_string(__rock_make_string("", 0)),
                         .Mode = mode};
  file res = allocate_compiler_persistent(sizeof(struct file));
  *res = tmp_res;
  if (mode == READ_MODE)
    res->Data = read_file(filename);
  else if (mode != WRITE_MODE) {
    print(new_string(__rock_make_string("Could not open file \'", 21)));
    print(filename);
    print(new_string(__rock_make_string("\': Invalid mode specified\n", 26)));
    exit(1);
  }
  return res;
}

void close_file(file f) {
  if (f->Mode == WRITE_MODE)
    write_string_to_file(f->Data, f->Filename);
  f->Data = new_string(__rock_make_string("", 0));
  f->Filename = new_string(__rock_make_string("", 0));
  f->Mode = CLOSED;
}

string get_file_contents(file f) {
  if (f->Mode != READ_MODE) {
    print_error();
    print(new_string(__rock_make_string(
        "Cannot access contents of a file that is not opened or not readable\n",
        68)));
    exit(1);
  }
  return f->Data;
}

void pfile(file f, string s) { f->Data = concat(f->Data, s); }

enum term_color { ROCKER_WHITE, ROCKER_RED, ROCKER_GREEN, ROCKER_YELLOW };
void print_color(string src, term_color col) {
  if (col == ROCKER_WHITE)
    print(new_string(__rock_make_string("\e[1;37m", 8)));
  else if (col == ROCKER_RED)
    print(new_string(__rock_make_string("\e[1;31m", 8)));
  else if (col == ROCKER_GREEN)
    print(new_string(__rock_make_string("\e[1;42m", 8)));
  else if (col == ROCKER_YELLOW)
    print(new_string(__rock_make_string("\e[1;43m", 8)));
  print(src);
  print(new_string(__rock_make_string("\e[0m", 5)));
}

void print_underline(string s) {
  print(new_string(__rock_make_string("\e[4m", 5)));
  print(s);
  print(new_string(__rock_make_string("\e[0m", 5)));
}

void print_info() {
  print(new_string(__rock_make_string("\e[1;32m", 8)));
  print(new_string(__rock_make_string("[INFO] ", 7)));
  print(new_string(__rock_make_string("\e[0m", 5)));
}

void print_cmd() {
  print(new_string(__rock_make_string("\e[1;33m", 8)));
  print(new_string(__rock_make_string("[CMD]  ", 7)));
  print(new_string(__rock_make_string("\e[0m", 5)));
}

void print_error() {
  print(new_string(__rock_make_string("\e[1;31m", 8)));
  print(new_string(__rock_make_string("[ERROR] ", 8)));
  print(new_string(__rock_make_string("\e[0m", 5)));
}

void implemented(string s) {
  print(new_string(__rock_make_string("TODO: ", 6)));
  print(s);
  print(new_string(__rock_make_string(" is not yet implemented\n", 24)));
  exit(1);
}

void compiler_assert(boolean b, string s) {
  if (b == 0) {
    print(new_string(__rock_make_string("Assertion failed: ", 18)));
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
  string res = new_string(__rock_make_string("", 0));
  for (int i = 0; i <= (int)length(chars) - 1; i++)
    res = concat(res, char_get_elem(chars, length(chars) - 1 - i));
  ;
  return res;
}

string toString(int n) { return string_of_int(n); }

string create_string(string src, int offset, int length) {
  string res = new_string(__rock_make_string("", 0));
  for (int i = 0; i <= (int)length - 1; i++) {
    char c = get_nth_char(src, offset + i);
    res = concat(res, c);
  };
  return res;
}

string cons_str(string src, int offset) {
  return create_string(src, offset, get_string_length(src) - offset);
}

struct Person {
  string name;
  int age;
};
struct Team {
  string leader;
  __internal_dynamic_array_t members;
};
int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    print(new_string(
        __rock_make_string("=== Test 1: Simple string concat ===\n", 37)));
    {
      string s1 = new_string(__rock_make_string("hello", 5));
      string s2 = concat(s1, new_string(__rock_make_string(" world", 6)));
      print(s2);
      print(new_string(__rock_make_string("\n", 1)));
    }
    print(new_string(
        __rock_make_string("=== Test 2: Dynamic string arrays ===\n", 38)));
    {
      __internal_dynamic_array_t names =
          __internal_make_array(sizeof(string), 0);
      string_push_array(names, new_string(__rock_make_string("Alice", 5)));
      string_push_array(names, new_string(__rock_make_string("Bob", 3)));
      string_push_array(names, new_string(__rock_make_string("Charlie", 7)));
      print(string_get_elem(names, 0));
      print(new_string(__rock_make_string(", ", 2)));
      print(string_get_elem(names, 1));
      print(new_string(__rock_make_string(", ", 2)));
      print(string_get_elem(names, 2));
      print(new_string(__rock_make_string("\n", 1)));
    }
    print(new_string(__rock_make_string(
        "=== Test 3: String concatenation in loop ===\n", 45)));
    {
      for (int i = 0; i <= (int)2; i++) {
        string s =
            concat(new_string(__rock_make_string("Index ", 6)), toString(i));
        print(s);
        print(new_string(__rock_make_string("\n", 1)));
      };
    }
    print(new_string(
        __rock_make_string("=== Test 4: Records with strings ===\n", 37)));
    {
      struct Person tmp_p1 = {
          .name = new_string(__rock_make_string("Alice", 5)), .age = 30};
      Person p1 = allocate_compiler_persistent(sizeof(struct Person));
      *p1 = tmp_p1;
      struct Person tmp_p2 = {.name = new_string(__rock_make_string("Bob", 3)),
                              .age = 25};
      Person p2 = allocate_compiler_persistent(sizeof(struct Person));
      *p2 = tmp_p2;
      print(p1->name);
      print(new_string(__rock_make_string(" is ", 4)));
      print(toString(p1->age));
      print(new_string(__rock_make_string("\n", 1)));
      print(p2->name);
      print(new_string(__rock_make_string(" is ", 4)));
      print(toString(p2->age));
      print(new_string(__rock_make_string("\n", 1)));
    }
    print(new_string(__rock_make_string(
        "=== Test 5: Records with dynamic string arrays ===\n", 51)));
    {
      struct Team tmp_team = {
          .leader = new_string(__rock_make_string("Alice", 5)),
          .members = __internal_make_array(sizeof(string), 0)};
      Team team = allocate_compiler_persistent(sizeof(struct Team));
      *team = tmp_team;
      string_push_array(team->members,
                        new_string(__rock_make_string("Alice", 5)));
      string_push_array(team->members,
                        new_string(__rock_make_string("Bob", 3)));
      string_push_array(team->members,
                        new_string(__rock_make_string("Charlie", 7)));
      print(new_string(__rock_make_string("Leader: ", 8)));
      print(team->leader);
      print(new_string(__rock_make_string("\n", 1)));
      print(new_string(__rock_make_string("Members: ", 9)));
      print(string_get_elem(team->members, 0));
      print(new_string(__rock_make_string(", ", 2)));
      print(string_get_elem(team->members, 1));
      print(new_string(__rock_make_string(", ", 2)));
      print(string_get_elem(team->members, 2));
      print(new_string(__rock_make_string("\n", 1)));
    }
    print(new_string(
        __rock_make_string("=== Test 6: Nested scopes with concat ===\n", 42)));
    {
      for (int i = 0; i <= (int)2; i++) {
        string prefix =
            concat(new_string(__rock_make_string("Item ", 5)), toString(i));
        print(prefix);
        print(new_string(__rock_make_string("\n", 1)));
      };
    }
    print(new_string(__rock_make_string(
        "=== Test 7: Dynamic array append and reassign ===\n", 50)));
    {
      __internal_dynamic_array_t items =
          __internal_make_array(sizeof(string), 0);
      string_push_array(items, new_string(__rock_make_string("first", 5)));
      print(new_string(__rock_make_string("Initial: ", 9)));
      print(string_get_elem(items, 0));
      print(new_string(__rock_make_string("\n", 1)));
      string_set_elem(items, (size_t)(0),
                      new_string(__rock_make_string("second", 6)));
      print(new_string(__rock_make_string("After reassign: ", 16)));
      print(string_get_elem(items, 0));
      print(new_string(__rock_make_string("\n", 1)));
    }
    print(new_string(__rock_make_string(
        "=== Test 8: String concatenation chain ===\n", 43)));
    {
      string result = concat(new_string(__rock_make_string("Hello", 5)),
                             new_string(__rock_make_string(" ", 1)));
      result = concat(result, new_string(__rock_make_string("World", 5)));
      result = concat(result, new_string(__rock_make_string("!", 1)));
      print(result);
      print(new_string(__rock_make_string("\n", 1)));
    }
    print(new_string(
        __rock_make_string("=== Test 9: Deep nesting with concat ===\n", 41)));
    {
      for (int i = 0; i <= (int)1; i++) {
        for (int j = 0; j <= (int)1; j++) {
          string msg =
              concat(new_string(__rock_make_string("i=", 2)), toString(i));
          msg = concat(msg, new_string(__rock_make_string(", j=", 4)));
          msg = concat(msg, toString(j));
          print(msg);
          print(new_string(__rock_make_string("\n", 1)));
        };
      };
    }
    print(new_string(__rock_make_string(
        "=== Test 10: Many iterations (loop allocation reuse) ===\n", 57)));
    {
      for (int i = 0; i <= (int)9; i++) {
        string s =
            concat(new_string(__rock_make_string("Count: ", 7)), toString(i));
        print(s);
        print(new_string(__rock_make_string("\n", 1)));
      };
    }
    print(new_string(
        __rock_make_string("=== All memory tests complete ===\n", 34)));
  }
  kill_compiler_stack();
  return 0;
}
