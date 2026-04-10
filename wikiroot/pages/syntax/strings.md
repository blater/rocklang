---
title: String Operations
category: syntax
tags: [string, concat, substring, to_string, char]
sources: []
updated: 2026-04-10
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
string greeting := "Hello, World!";
string empty := "";
```

String literals are initialised via `__rock_make_string()` at runtime. The generator emits setup code into the `pre_f` buffer.

## concat

```rock
concat(s1, s2)
```

Concatenate two strings (or a string and a char). Returns a new string.

```rock
string s := concat("Hello, ", "World!");
string s2 := concat("Score: ", to_string(42));
```

The generator infers whether the second argument is a `char` or `string` and calls `__concat_char` or `__concat_str` accordingly.

## substring

```rock
substring(s, from)           -- from index to end
substring(s, from, length)   -- from index, given length
```

```rock
string s := "Hello, World!";
string world := substring(s, 8);      -- "World!"
string hello := substring(s, 1, 5);   -- "Hello"
```

## to_string

```rock
to_string(n)   -- works for int, byte, word, dword
```

```rock
string s := to_string(42);            -- "42"
string b := to_string(to_byte(255));  -- "255"
```

## get_nth_char / set_nth_char

```rock
get_nth_char(s, index)          -- returns char at index
set_nth_char(s, index, c)       -- mutate char at index
```

```rock
char c := get_nth_char("hello", 0);   -- 'h'
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
char c := 'A';
```

Single characters use single quotes. Concatenating a char with a string uses `__concat_char()`.

See [[syntax/types]] for type declarations, [[syntax/arrays]] for string arrays, and [[concepts/string-representation]] for the runtime struct.
