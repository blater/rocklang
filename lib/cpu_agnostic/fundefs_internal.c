#include "fundefs_internal.h"

#include "alloc.h"
#include "fundefs.h"
#include "typedefs.h"
#include <stdio.h>
#include <string.h>

__internal_dynamic_array_t __internal_make_array(size_t size, size_t max_capacity) {
  __internal_dynamic_array_t ptr =
      allocate_compiler_persistent(sizeof(struct __internal_dynamic_array));
  if (size == 0) {
    printf("Could not pop elem out of dynamic array: BAD ELEMENT SIZE\n");
    return ptr;
  }
  (*ptr).elem_size = size;
  (*ptr).max_capacity = max_capacity;

  // For fixed-size arrays, allocate exactly the requested size
  // For dynamic arrays (max_capacity == 0), use default capacity
  if (max_capacity > 0) {
    (*ptr).capacity = max_capacity;
  } else {
    (*ptr).capacity = __INTERNAL_DYNAMIC_ARRAY_CAP;
  }

  (*ptr).data = allocate_compiler_persistent(ptr->elem_size * ptr->capacity);
  (*ptr).length = 0;
  return ptr;
}

// We use the compiler version of thre
//  allocator as well for convenience
// but this may change btw reallocation
// does not change the scope in which
// the pointer was allocated
int __internal_push_array(__internal_dynamic_array_t arr, void *elem) {
  if (arr->data == NULL) {
    printf("Uninitialized array !\n");
    exit_rocker(1);
  }
  if (elem == NULL) {
    printf("Could not push elem to dynamic array: BAD ELEM\n");
    exit_rocker(1);
  }

  // Check if we can add more elements
  if (arr->max_capacity > 0 && arr->length >= arr->max_capacity) {
    printf("Error: Cannot append to fixed-size array (capacity: %zu, length: %zu)\n",
           arr->max_capacity, arr->length);
    exit_rocker(1);
  }

  if (arr->length > arr->capacity) {
    // For dynamic arrays only, grow the capacity
    if (arr->max_capacity == 0) {
      arr->capacity *= 2;
      arr->data = reallocate_compiler_persistent(arr->data,
                                                 arr->capacity * arr->elem_size);
    } else {
      printf("Error: Array capacity exceeded\n");
      exit_rocker(1);
    }
  }
  void *dst = arr->data + arr->length * arr->elem_size;
  if (dst == NULL) {
    printf("Could not push elem to dynamic array: BAD ARRAY\n");
    exit_rocker(1);
  }
  // memccpy(dst, elem, 1, arr->elem_size);
  for (int i = 0; i < arr->elem_size; i++) {
    ((char *)dst)[i] = ((char *)elem)[i];
  }
  arr->length++;
  return 0;
}

void *__internal_pop_array(__internal_dynamic_array_t arr) {
  if (arr->length == 0) {
    printf("Could not pop elem out of dynamic array: EMPTY ARRAY\n");

    return NULL;
  }
  if (arr->elem_size == 0) {
    printf("Could not pop elem out of dynamic array: BAD ELEMENT SIZE\n");
    return NULL;
  }
  void *res = allocate_compiler_persistent(arr->elem_size);
  void *src = arr->data + (arr->length - 1) * arr->elem_size;
  memcpy(res, src, arr->elem_size);
  arr->length--;
  return res;
}

void *__internal_get_elem(__internal_dynamic_array_t arr, size_t index) {
  // For fixed-size arrays, check against capacity; for dynamic, check against length
  size_t limit = (arr->max_capacity > 0) ? arr->max_capacity : arr->length;
  if (index >= limit) {
    printf("Could not get elem from dynamic array: INDEX OUT OF BOUNDS (%ld)\n",
           index);

    int *tmp = NULL;
    *tmp = 3;
    return NULL;
  }
  if (arr->elem_size == 0) {
    printf("Could not get elem from dynamic array: BAD ELEMENT SIZE\n");
    return NULL;
  }
  // void *res = allocate_compiler_persistent(arr->elem_size);
  void *src = arr->data + index * arr->elem_size;
  if (src == NULL)
    return NULL;
  return src;
}

void __internal_insert(__internal_dynamic_array_t arr, size_t index,
                       void *elem) {}

void __internal_set_elem(__internal_dynamic_array_t arr, size_t index,
                         void *elem) {
  // For fixed-size arrays, check against capacity; for dynamic, check against length
  size_t limit = (arr->max_capacity > 0) ? arr->max_capacity : arr->length;
  if (index >= limit) {
    printf("Could not set elem in dynamic array: INDEX OUT OF BOUNDS (%zu, limit: %zu)\n",
           index, limit);
    exit_rocker(1);
  }
  if (arr->elem_size == 0) {
    printf("Could not set elem in dynamic array: BAD ELEMENT SIZE\n");
    exit_rocker(1);
  }
  void *dst = arr->data + index * arr->elem_size;
  if (dst == NULL) {
    printf("Could not set elem in dynamic array: BAD ARRAY\n");
    exit_rocker(1);
  }
  memcpy(dst, elem, arr->elem_size);
  // Update length if we're setting beyond current length (for fixed-size arrays)
  if (index >= arr->length) {
    arr->length = index + 1;
  }
}

size_t __length_array(__internal_dynamic_array_t arr) {
  if (arr == NULL)
    return 0;
  return arr->length;
}

size_t __length_string(string s) {
  return s.length;
}

__internal_dynamic_array_t int_make_array(void) {
  return __internal_make_array(sizeof(int), 0);
}

void int_push_array(__internal_dynamic_array_t arr, int elem) {
  __internal_push_array(arr, &elem);
}

int int_pop_array(__internal_dynamic_array_t arr) {
  int *res = __internal_pop_array(arr);
  return *res;
}

int int_get_elem(__internal_dynamic_array_t arr, size_t index) {
  int *res = __internal_get_elem(arr, index);
  return *res;
}

void int_set_elem(__internal_dynamic_array_t arr, size_t index, int elem) {
  __internal_set_elem(arr, index, &elem);
}

void int_insert(__internal_dynamic_array_t arr, size_t index, int elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t boolean_make_array(void) {
  return __internal_make_array(sizeof(boolean), 0);
}

void boolean_push_array(__internal_dynamic_array_t arr, boolean elem) {
  __internal_push_array(arr, &elem);
}

boolean boolean_pop_array(__internal_dynamic_array_t arr) {
  boolean *res = __internal_pop_array(arr);
  return *res;
}

boolean boolean_get_elem(__internal_dynamic_array_t arr, size_t index) {
  boolean *res = __internal_get_elem(arr, index);
  return *res;
}

void boolean_set_elem(__internal_dynamic_array_t arr, size_t index,
                      boolean elem) {
  __internal_set_elem(arr, index, &elem);
}

void boolean_insert(__internal_dynamic_array_t arr, size_t index,
                    boolean elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t string_make_array(void) {
  return __internal_make_array(sizeof(string), 0);
}

void string_push_array(__internal_dynamic_array_t arr, string elem) {
  string copy;
  new_string(&copy, elem);
  __internal_push_array(arr, &copy);
}

void string_pop_array(string *out, __internal_dynamic_array_t arr) {
#ifdef __SDCC
  (void)arr;
  __rock_make_string(out, "", 0);
#else
  string *res = __internal_pop_array(arr);
  new_string(out, *res);
#endif
}

void string_get_elem(string *out, __internal_dynamic_array_t arr, size_t index) {
#ifdef __SDCC
  (void)arr; (void)index;
  __rock_make_string(out, "", 0);
#else
  string *tmp = __internal_get_elem(arr, index);
  new_string(out, *tmp);
#endif
}

void string_set_elem(__internal_dynamic_array_t arr, size_t index,
                     string elem) {
  // Free old value before overwriting (if not NULL)
  string old;
  string_get_elem(&old, arr, index);
  if (old.data != NULL) {
    // Old value will be overwritten; in a full implementation,
    // we would free its persistent allocation here
  }
  string copy;
  new_string(&copy, elem);
  __internal_set_elem(arr, index, &copy);
}

void string_insert(__internal_dynamic_array_t arr, size_t index, string elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t char_make_array(void) {
  return __internal_make_array(sizeof(char), 0);
}

void char_push_array(__internal_dynamic_array_t arr, char elem) {
  __internal_push_array(arr, &elem);
}

char char_pop_array(__internal_dynamic_array_t arr) {
  char *res = __internal_pop_array(arr);
  return *res;
}

char char_get_elem(__internal_dynamic_array_t arr, size_t index) {
  char *res = __internal_get_elem(arr, index);
  return *res;
}

void char_set_elem(__internal_dynamic_array_t arr, size_t index, char elem) {
  __internal_set_elem(arr, index, &elem);
}

void char_insert(__internal_dynamic_array_t arr, size_t index, char elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t byte_make_array(void) {
  return __internal_make_array(sizeof(byte), 0);
}

void byte_push_array(__internal_dynamic_array_t arr, byte elem) {
  __internal_push_array(arr, &elem);
}

byte byte_pop_array(__internal_dynamic_array_t arr) {
  byte *res = __internal_pop_array(arr);
  return *res;
}

byte byte_get_elem(__internal_dynamic_array_t arr, size_t index) {
  byte *res = __internal_get_elem(arr, index);
  return *res;
}

void byte_set_elem(__internal_dynamic_array_t arr, size_t index, byte elem) {
  __internal_set_elem(arr, index, &elem);
}

void byte_insert(__internal_dynamic_array_t arr, size_t index, byte elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t word_make_array(void) {
  return __internal_make_array(sizeof(word), 0);
}
void word_push_array(__internal_dynamic_array_t arr, word elem) {
  __internal_push_array(arr, &elem);
}
word word_pop_array(__internal_dynamic_array_t arr) {
  word *res = __internal_pop_array(arr);
  return *res;
}
word word_get_elem(__internal_dynamic_array_t arr, size_t index) {
  word *res = __internal_get_elem(arr, index);
  return *res;
}
void word_set_elem(__internal_dynamic_array_t arr, size_t index, word elem) {
  __internal_set_elem(arr, index, &elem);
}
void word_insert(__internal_dynamic_array_t arr, size_t index, word elem) {
  __internal_insert(arr, index, &elem);
}

__internal_dynamic_array_t dword_make_array(void) {
  return __internal_make_array(sizeof(dword), 0);
}
void dword_push_array(__internal_dynamic_array_t arr, dword elem) {
  __internal_push_array(arr, &elem);
}
dword dword_pop_array(__internal_dynamic_array_t arr) {
  dword *res = __internal_pop_array(arr);
  return *res;
}
dword dword_get_elem(__internal_dynamic_array_t arr, size_t index) {
  dword *res = __internal_get_elem(arr, index);
  return *res;
}
void dword_set_elem(__internal_dynamic_array_t arr, size_t index, dword elem) {
  __internal_set_elem(arr, index, &elem);
}
void dword_insert(__internal_dynamic_array_t arr, size_t index, dword elem) {
  __internal_insert(arr, index, &elem);
}

int global_argc;
char **global_argv;

void fill_cmd_args(int argc, char **argv) {
  global_argc = argc;
  global_argv = argv;
}

__internal_dynamic_array_t get_args(void) {
  __internal_dynamic_array_t cmd_args = string_make_array();
  for (int i = 0; i < global_argc; i++) {
    string arg;
    cstr_to_string(&arg, global_argv[i]);
    string_push_array(cmd_args, arg);
    // printf("Pushing arg \'");
    // print(arg);
    // printf("\'\n");
  }
  return cmd_args;
}

void init_rocker(int argc, char **argv) {
  init_compiler_stack();
  fill_cmd_args(argc, argv);
}

void end_rocker(void) {
  kill_compiler_stack();
  //
}

void exit_rocker(int status) {
  end_rocker();
  exit(status);
}