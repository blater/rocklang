#ifndef ROCKER_TYPEDEFS_INTERNAL
#define ROCKER_TYPEDEFS_INTERNAL

#include <stddef.h>
#include <stdint.h>

typedef struct string {
  char *data;
  size_t length;
} string;

typedef struct __internal_dynamic_array *__internal_dynamic_array_t;

struct __internal_dynamic_array {
  void *data;
  size_t length;
  size_t capacity;
  size_t elem_size;
  size_t max_capacity;  // 0 = unlimited (dynamic), >0 = fixed size limit
};

typedef char boolean;
typedef unsigned char byte;
typedef unsigned short word;
typedef uint32_t dword;
#define true 1
#define false 0

#endif // ROCKER_TYPEDEFS_INTERNAL
