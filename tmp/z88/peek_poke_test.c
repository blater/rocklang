#include "../../src/generation/fundefs.h"
#include "../../src/generation/fundefs_internal.h"
#include "../../src/generation/typedefs.h"

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
    print(new_string(__rock_make_string("Testing peek/poke functions\n", 28)));
    print(new_string(__rock_make_string("Test 1: Basic poke/peek\n", 24)));
    poke(to_word(1000), to_byte(42));
    byte read_val = peek(to_word(1000));
    print(new_string(
        __rock_make_string("  poke(1000, 42) -> peek(1000) = ", 33)));
    print(to_string(to_int(read_val)));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(
        __rock_make_string("Test 2: Multiple consecutive pokes\n", 35)));
    poke(to_word(2000), to_byte(100));
    poke(to_word(2001), to_byte(200));
    poke(to_word(2002), to_byte(150));
    byte val1 = peek(to_word(2000));
    byte val2 = peek(to_word(2001));
    byte val3 = peek(to_word(2002));
    print(new_string(__rock_make_string("  poke(2000,100) peek(2000)=", 28)));
    print(to_string(to_int(val1)));
    print(new_string(__rock_make_string(" poke(2001,200) peek(2001)=", 27)));
    print(to_string(to_int(val2)));
    print(new_string(__rock_make_string(" poke(2002,150) peek(2002)=", 27)));
    print(to_string(to_int(val3)));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(__rock_make_string("Test 3: Zero value\n", 19)));
    poke(to_word(3000), to_byte(0));
    byte zero_val = peek(to_word(3000));
    print(
        new_string(__rock_make_string("  poke(3000, 0) -> peek(3000) = ", 32)));
    print(to_string(to_int(zero_val)));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(__rock_make_string("Test 4: Max byte value (255)\n", 29)));
    poke(to_word(4000), to_byte(255));
    byte max_val = peek(to_word(4000));
    print(new_string(
        __rock_make_string("  poke(4000, 255) -> peek(4000) = ", 34)));
    print(to_string(to_int(max_val)));
    print(new_string(__rock_make_string("\n", 1)));
    print(new_string(__rock_make_string("peek/poke test complete\n", 24)));
  }
  kill_compiler_stack();
  return 0;
}
