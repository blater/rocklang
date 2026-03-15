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

typedef struct array_wrapper *array_wrapper;
array_wrapper func();

__internal_dynamic_array_t array_wrapper_make_array(void) {
  return __internal_make_array(sizeof(array_wrapper), 0);
}

void array_wrapper_push_array(__internal_dynamic_array_t arr,
                              array_wrapper elem) {
  __internal_push_array(arr, &elem);
}

array_wrapper array_wrapper_pop_array(__internal_dynamic_array_t arr) {
  array_wrapper *res = __internal_pop_array(arr);
  return *res;
}

array_wrapper array_wrapper_get_elem(__internal_dynamic_array_t arr,
                                     size_t index) {
  array_wrapper *res = __internal_get_elem(arr, index);
  if (res == NULL) {
    printf("NULL ELEMENT IN array_wrapper_get_elem");
    exit(1);
  }
  return *res;
}

void array_wrapper_set_elem(__internal_dynamic_array_t arr, size_t index,
                            array_wrapper elem) {
  __internal_set_elem(arr, index, &elem);
}

void array_wrapper_insert(__internal_dynamic_array_t arr, size_t index,
                          array_wrapper elem) {
  __internal_insert(arr, index, &elem);
}

struct array_wrapper {
  __internal_dynamic_array_t Items;
  __internal_dynamic_array_t Names;
};
array_wrapper func() {
  struct array_wrapper tmp_a = {.Items = NULL, .Names = NULL};
  array_wrapper a = allocate_compiler_persistent(sizeof(struct array_wrapper));
  *a = tmp_a;
  __internal_dynamic_array_t items = __internal_make_array(sizeof(int), 0);
  a->Items = items;
  __internal_dynamic_array_t names = __internal_make_array(sizeof(string), 0);
  a->Names = names;
  int_push_array(a->Items, 1);
  string_push_array(names,
                    new_string((string){.data = "First\n", .length = 6}));
  int_push_array(a->Items, 2);
  string_push_array(names,
                    new_string((string){.data = "Second\n", .length = 7}));
  int_push_array(a->Items, 3);
  string_push_array(names,
                    new_string((string){.data = "Third\n", .length = 6}));
  return a;
}

int main(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
  {
    array_wrapper a = func();
    __internal_dynamic_array_t names = a->Names;
    print(new_string((string){.data = "\n", .length = 1}));
    for (int __iter_i = 0; __iter_i < names->length; __iter_i++) {
      string name = ((string *)names->data)[__iter_i];
      {
        print(name);
      }
    }
    struct array_wrapper tmp_new_arr = {
        .Items = __internal_make_array(sizeof(int), 0),
        .Names = __internal_make_array(sizeof(string), 0)};
    array_wrapper new_arr =
        allocate_compiler_persistent(sizeof(struct array_wrapper));
    *new_arr = tmp_new_arr;
    print(new_string((string){.data = "Done\n", .length = 5}));
  }
  kill_compiler_stack();
  return 0;
}
