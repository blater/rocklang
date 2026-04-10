---
title: Array Internals
category: concepts
tags: [array, dynamic-array, fixed-array, runtime, fundefs_internal]
sources: []
updated: 2026-04-09
status: current
---

# Array Internals

All Rock arrays — dynamic and fixed-size — share a single generic runtime struct with type-specific wrapper functions synthesised by the generator.

## Runtime Struct

```c
typedef struct __internal_dynamic_array {
  void   *data;           // Heap-allocated element storage
  size_t  length;         // Current number of elements
  size_t  capacity;       // Allocated capacity (in elements)
  size_t  elem_size;      // Size of one element (sizeof(T))
  size_t  max_capacity;   // 0 = dynamic; >0 = fixed-size cap
} __internal_dynamic_array_t;
```

## Generic Operations (fundefs_internal.c)

These operate on raw `void*` element pointers. The type-specific wrappers call these:

| Function | Signature | Behaviour |
|----------|-----------|-----------|
| `__internal_make_array` | `(elem_size, max_capacity) → arr` | Allocate with initial capacity 4 (or max if fixed) |
| `__internal_push_array` | `(arr*, elem*) → void` | Copy elem onto end; grow if needed |
| `__internal_pop_array` | `(arr*) → void*` | Decrement length; return pointer to (now-invalid) slot |
| `__internal_get_elem` | `(arr*, idx) → void*` | Bounds-check; return pointer to element |
| `__internal_set_elem` | `(arr*, idx, elem*) → void` | Bounds-check; memcpy elem into slot |
| `__internal_insert` | `(arr*, idx, elem*) → void` | Shift right from idx; copy elem in; increment length |

### Growth Strategy

Dynamic arrays double capacity on overflow:
```c
if (arr->length >= arr->capacity) {
  arr->capacity *= 2;
  arr->data = realloc(arr->data, arr->capacity * arr->elem_size);
}
```

Fixed-size arrays check `max_capacity` before append/insert and halt the program if exceeded:
```c
if (arr->max_capacity > 0 && arr->length >= arr->max_capacity) {
  fprintf(stderr, "Error: array capacity exceeded\n");
  exit(1);
}
```

### Bounds Checking

`__internal_get_elem` and `__internal_set_elem` both bounds-check against `length`:
```c
if (idx < 0 || idx >= arr->length) {
  fprintf(stderr, "Error: array index out of bounds\n");
  exit(1);
}
```

### Insert Implementation

`__internal_insert` shifts elements right to make room:
```c
void __internal_insert(arr, index, elem) {
  // Grow if needed
  // for i from length down to index+1: copy [i-1] → [i]
  // memcpy elem into [index]
  // arr->length++
}
```

## Type-Specific Wrappers

The generator synthesises one set of wrappers per element type encountered. For element type `T`:

```c
__internal_dynamic_array_t T_make_array(void) {
  return __internal_make_array(sizeof(T), 0);
}

void T_push_array(__internal_dynamic_array_t *arr, T elem) {
  __internal_push_array(arr, &elem);
}

T T_pop_array(__internal_dynamic_array_t *arr) {
  void *ptr = __internal_pop_array(arr);
  return *(T*)ptr;
}

T T_get_elem(__internal_dynamic_array_t *arr, int idx) {
  return *(T*)__internal_get_elem(arr, idx);
}

void T_set_elem(__internal_dynamic_array_t *arr, int idx, T elem) {
  __internal_set_elem(arr, idx, &elem);
}

void T_insert(__internal_dynamic_array_t *arr, int idx, T elem) {
  __internal_insert(arr, idx, &elem);
}
```

### String Special Case

`string_push_array` and `string_set_elem` perform a **deep copy** before insertion to prevent aliasing (important because `rock_string.data` is a pointer):

```c
void string_push_array(__internal_dynamic_array_t *arr, rock_string elem) {
  rock_string copy;
  copy.data = allocate_compiler_persistent(elem.length + 1);
  memcpy(copy.data, elem.data, elem.length + 1);
  copy.length = elem.length;
  __internal_push_array(arr, &copy);
}
```

## Memory Model

Array storage (`data` pointer) is allocated via `allocate_compiler_persistent()`. The arena model means no array is ever explicitly freed — memory persists until `kill_compiler_stack()` at program exit.

## Element Type Inference in Generator

To select the correct wrapper, the generator must know the element type of an array at each use site. It uses `get_array_element_type()` which:
1. Looks up the array variable in the name table
2. Examines the `ast_vardef` or `ast_fundef` node for the type annotation
3. Returns the element type as a string (e.g. `"int"`, `"string"`)

This is called at each `append`, `get`, `set`, `pop`, `insert` site. If inference fails (unknown type), the generator falls back to a default wrapper name, which may produce a C type error.

See [[generator/generator-overview]] for array wrapper synthesis, [[syntax/arrays]] for Rock-level array syntax, and [[concepts/string-representation]] for string-specific deep copy behaviour.
