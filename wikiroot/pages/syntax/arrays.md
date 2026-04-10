---
title: Arrays
category: syntax
tags: [arrays, dynamic-array, fixed-array, append, insert, get, set, pop, length]
sources: []
updated: 2026-04-10
status: current
---

# Arrays

## Declaration

```rock
int[]      -- dynamic array of int (grows automatically)
string[10] -- fixed-size array of 10 strings (capacity capped)
```

```rock
int[] nums := [];       -- empty dynamic array
string[5] strs;         -- fixed-size, default-initialised
int[] xs := [];
```

## Built-in Array Operations

### append
```rock
append(arr, value)
```
Add `value` to the end of `arr`. For dynamic arrays, capacity doubles if needed. For fixed-size arrays, halts if at capacity.

```rock
int[] nums := [];
append(nums, 1);
append(nums, 2);
append(nums, 3);
-- nums is now [1, 2, 3]
```

### get
```rock
get(arr, index)
```
Return the element at zero-based `index`. Out-of-bounds access prints an error and exits.

```rock
int first := get(nums, 0);   -- 1
```

### set
```rock
set(arr, index, value)
```
Replace the element at `index` with `value`.

```rock
set(nums, 1, 99);   -- nums is now [1, 99, 3]
```

### pop
```rock
pop(arr)
```
Remove and return the last element. Decrements length.

```rock
int last := pop(nums);   -- returns 3, nums is now [1, 99]
```

### insert
```rock
insert(arr, index, value)
```
Insert `value` at `index`, shifting all elements from `index` onwards one position to the right. Length increases by 1.

```rock
insert(nums, 0, 42);   -- [42, 1, 99]
insert(nums, 2, 55);   -- [42, 1, 55, 99]
```

### length
```rock
length(arr)
```
Return the current number of elements.

```rock
int n := length(nums);
```

## Array Indexing (direct)

```rock
arr[idx]         -- read element at idx (same as get)
arr[idx] := val  -- set element at idx (same as set)
```

Array expressions can come from record fields or other postfix expressions, not just plain variables:

```rock
record Buffer { int[] Items }

Buffer b := { Items := [] };
append(b.Items, 10);
int first := b.Items[0];
int alsoFirst := get(b.Items, 0);
```

## Iterator Loop

```rock
for elem in arr {
  -- use elem
}
```

Iterates over all elements. `elem` is scoped to the loop body. See [[syntax/control-flow]] for full loop syntax.

## String Arrays

String arrays deep-copy strings on insert and push to prevent aliasing:

```rock
string[] words := [];
append(words, "hello");
append(words, "world");
string w := get(words, 0);   -- "hello"
```

## Fixed-Size Arrays

```rock
int[5] data;   -- exactly 5 ints, default 0
```

Fixed-size arrays use the same `__internal_dynamic_array_t` struct as dynamic arrays but with `max_capacity` set. Attempting to `append` or `insert` beyond capacity prints an error and halts.

## Runtime Representation

All arrays are `__internal_dynamic_array_t`:
```c
typedef struct {
  void   *data;
  size_t  length;
  size_t  capacity;
  size_t  elem_size;
  size_t  max_capacity;   // 0 = dynamic; >0 = fixed-size
} __internal_dynamic_array_t;
```

Type-specific wrapper functions (`int_push_array`, `string_get_elem`, etc.) are synthesised by the generator for each element type used. See [[concepts/array-internals]] for the full runtime implementation.
