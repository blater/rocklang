---
title: String Operations
category: syntax
tags: [string, concat, substring, to_string, char]
sources: []
updated: 2026-04-09
status: current
---

# String Operations

## String Type

Rock strings are represented as a two-field struct:
```c
typedef struct { char *data; size_t length; } rock_string;
```

Strings are null-terminated (for C interop) but length is also tracked. See [[concepts/string-representation]] for the full runtime detail.

## Literals

```rock
let greeting: string := "Hello, World!";
let empty: string := "";
```

String literals are initialised via `__rock_make_string()` at runtime. The generator emits setup code into the `pre_f` buffer.

## concat

```rock
concat(s1, s2)
```

Concatenate two strings (or a string and a char). Returns a new string.

```rock
let s: string := concat("Hello, ", "World!");
let s2: string := concat("Score: ", to_string(42));
```

The generator infers whether the second argument is a `char` or `string` and calls `__concat_char` or `__concat_str` accordingly.

## substring

```rock
substring(s, from)           -- from index to end
substring(s, from, length)   -- from index, given length
```

```rock
let s: string := "Hello, World!";
let world: string := substring(s, 7);       -- "World!"
let hello: string := substring(s, 0, 5);   -- "Hello"
```

## to_string

```rock
to_string(n)   -- works for int, byte, word, dword
```

```rock
let s: string := to_string(42);    -- "42"
let b: string := to_string(255b);  -- "255"
```

## get_nth_char / set_nth_char

```rock
get_nth_char(s, index)          -- returns char at index
set_nth_char(s, index, c)       -- mutate char at index
```

```rock
let c: char := get_nth_char("hello", 0);   -- 'h'
```

## get_string_length

```rock
get_string_length(s)   -- returns int length
```

Equivalent to `length(s)` when `s` is a string.

## print / printf

```rock
print(s)          -- print a Rock string
printf("fmt %d", n)  -- C-style format print
```

`print` maps to `printf("%s", s.data)` in generated C. `printf` is forwarded directly to C's `printf`.

## Char Type

```rock
let c: char := 'A';
```

Single characters use single quotes. Concatenating a char with a string uses `__concat_char()`.

See [[syntax/types]] for type declarations, [[syntax/arrays]] for string arrays, and [[concepts/string-representation]] for the runtime struct.
