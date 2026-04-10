---
title: String Representation
category: concepts
tags: [string, rock_string, string-temporary, pre_f, string-literal]
sources: []
updated: 2026-04-10
status: current
---

# String Representation

## Runtime Struct

Rock strings are represented as `rock_string` — a two-field C struct:

```c
typedef struct {
  char   *data;    // Null-terminated character data
  size_t  length;  // Number of characters (not counting null terminator)
} rock_string;
```

Strings are both null-terminated (for C interop with `printf` and similar) and length-tracked (for efficient operations and to avoid O(n) `strlen` calls).

## String Literal Initialisation

In generated C, string literals cannot be assigned directly to `rock_string`. The generator emits setup code into the `pre_f` buffer:

```c
// For:   string s := "hello";
// pre_f:
rock_string __strtmp_0;
__rock_make_string(&__strtmp_0, "hello");
// f:
rock_string s = __strtmp_0;
```

`__rock_make_string(rock_string *dest, const char *src)` sets `dest->data = src` and `dest->length = strlen(src)`. The string data itself is the literal's storage in the C binary's read-only data segment.

## String Temporaries

Complex string expressions (concat, substring) require temporary variables. The generator allocates these as `__strtmp_N` where N increments per function:

```c
// For:  print(concat(a, concat(b, c)));
// pre_f:
rock_string __strtmp_0 = __concat_str(b, c);
rock_string __strtmp_1 = __concat_str(a, __strtmp_0);
// f:
printf("%s", __strtmp_1.data);
```

The counter `str_tmp_counter` in `generator_t` is reset at the start of each function.

## String Operations at Runtime

| Function | Signature | Notes |
|----------|-----------|-------|
| `__rock_make_string` | `(rock_string*, const char*)` | Init from literal |
| `__concat_str` | `(rock_string, rock_string) → rock_string` | String + string |
| `__concat_char` | `(rock_string, char) → rock_string` | String + char |
| `__substring_from` | `(rock_string, int) → rock_string` | From index to end |
| `__substring_range` | `(rock_string, int, int) → rock_string` | From index, N chars |
| `__length_string` | `(rock_string) → int` | Returns `.length` |
| `__to_string_int` | `(int) → rock_string` | Number to string |

All result strings are heap-allocated via the arena allocator (`allocate_compiler_persistent`).

## String Arrays

Strings stored in arrays are **deep-copied** on insert to prevent aliasing:

```c
// string_push_array copies the string's data before insertion
void string_push_array(__internal_dynamic_array_t *arr, rock_string elem) {
  rock_string copy;
  copy.data = allocate_compiler_persistent(elem.length + 1);
  memcpy(copy.data, elem.data, elem.length + 1);
  copy.length = elem.length;
  __internal_push_array(arr, &copy);
}
```

This ensures mutations to the original string don't corrupt the stored copy.

## Memory Lifecycle

All string data is allocated through the arena allocator. Strings are never freed individually — all memory is released at program exit via `kill_compiler_stack()`. This is safe for short-lived programs (typical for Rock's target use cases) but means long-running programs with many string allocations will grow memory continuously.

## Generator Type Inference for Strings

When the generator encounters a `concat(a, b)` call, it calls `infer_expr_type()` on `b` to determine whether to call `__concat_str` or `__concat_char`. If `b` resolves to `char`, `__concat_char` is used; otherwise `__concat_str`.

See [[generator/generator-overview]] for the `pre_f` buffer pattern, [[concepts/array-internals]] for string array storage, and [[ubiquitous-language]] for the `rock_string` definition.
