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

int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    print(new_string(
        __rock_make_string("=== Advanced Byte Type Tests ===\n", 33)));
    print(new_string(
        __rock_make_string("\n--- Test 1: Mixed Arithmetic ---\n", 34)));
    byte b1 = to_byte(10);
    int i1 = 20;
    int result = to_int(b1) + i1;
    print(new_string(__rock_make_string("to_int(10) + 20 = ", 18)));
    print(to_string(result));
    print(new_string(__rock_make_string("\n", 1)));
    byte b2 = to_byte(50);
    int result2 = to_int(b2) * 2;
    print(new_string(__rock_make_string("to_int(50) * 2 = ", 17)));
    print(to_string(result2));
    print(new_string(__rock_make_string("\n", 1)));
    print(
        new_string(__rock_make_string("\n--- Test 2: Comparisons ---\n", 29)));
    byte b3 = to_byte(100);
    byte b4 = to_byte(50);
    int i3 = to_int(b3);
    int i4 = to_int(b4);
    if (i3 > i4) {
      print(new_string(__rock_make_string("100 > 50: true\n", 15)));
    } else {
      print(new_string(__rock_make_string("100 > 50: false\n", 16)));
    }
    if (i4 < i3) {
      print(new_string(__rock_make_string("50 < 100: true\n", 15)));
    } else {
      print(new_string(__rock_make_string("50 < 100: false\n", 16)));
    }
    print(new_string(__rock_make_string(
        "\n--- Test 3: Loop with Byte Arithmetic ---\n", 43)));
    {
      for (int i = 0; i <= (int)4; i++) {
        byte b = to_byte(i * 10);
        print(new_string(__rock_make_string("i=", 2)));
        print(to_string(i));
        print(new_string(__rock_make_string(", b=", 4)));
        print(to_string(b));
        print(new_string(__rock_make_string("\n", 1)));
      };
    }
    print(new_string(
        __rock_make_string("\n--- Test 4: Byte Array Operations ---\n", 39)));
    __internal_dynamic_array_t arr = __internal_make_array(sizeof(byte), 0);
    {
      for (int j = 0; j <= (int)4; j++) {
        byte_push_array(arr, to_byte(j * 20));
      };
    }
    print(new_string(__rock_make_string("Array: ", 7)));
    {
      for (int k = 0; k <= (int)4; k++) {
        print(to_string(to_int(byte_get_elem(arr, k))));
        print(new_string(__rock_make_string(" ", 1)));
      };
    }
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(
        __rock_make_string("\n--- Test 5: Sum of Byte Array ---\n", 35)));
    int sum = 0;
    {
      for (int idx = 0; idx <= (int)4; idx++) {
        sum = sum + to_int(byte_get_elem(arr, idx));
      };
    }
    print(new_string(__rock_make_string("Sum of array: ", 14)));
    print(to_string(sum));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(__rock_make_string(
        "\n--- Test 6: Truncation in Expressions ---\n", 43)));
    int large = 1000;
    byte b_trunc = to_byte(large);
    print(new_string(__rock_make_string("to_byte(1000) = ", 16)));
    print(to_string(b_trunc));
    print(new_string(__rock_make_string(" (1000 mod 256 = 232)\n", 22)));
    print(new_string(__rock_make_string(
        "\n--- Test 7: String Concatenation with Bytes ---\n", 49)));
    byte b5 = to_byte(65);
    string s1 = new_string(__rock_make_string("Value: ", 7));
    string s2 = concat(s1, to_string(b5));
    print(s2);
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(__rock_make_string(
        "\n--- Test 8: Type Conversions in Chain ---\n", 43)));
    byte b6 = to_byte(42);
    int i2 = to_int(b6);
    byte b7 = to_byte(i2 + 8);
    print(new_string(__rock_make_string("Start: 42, +8 = ", 16)));
    print(to_string(b7));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(
        __rock_make_string("\n--- Test 9: Boundary Values ---\n", 33)));
    byte b_zero = to_byte(0);
    byte b_max = to_byte(255);
    print(new_string(__rock_make_string("Min byte: ", 10)));
    print(to_string(b_zero));
    print(new_string(__rock_make_string(", Max byte: ", 12)));
    print(to_string(b_max));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(
        __rock_make_string("\n--- Test 10: Wrap-around Arithmetic ---\n", 41)));
    byte b_almost_max = to_byte(250);
    byte b_add_result = to_byte(to_int(b_almost_max) + 10);
    print(new_string(__rock_make_string("250 + 10 = ", 11)));
    print(to_string(b_add_result));
    print(new_string(__rock_make_string(" (wraps to 4)\n", 14)));
    print(new_string(
        __rock_make_string("\n=== All Advanced Tests Complete ===\n", 37)));
  }
  kill_compiler_stack();
  return 0;
}
