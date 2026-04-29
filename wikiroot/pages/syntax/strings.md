---
title: String Operations
category: syntax
tags: [string, concat, substring, toString, char]
sources: []
updated: 2026-04-25
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
string s2 := concat("Score: ", toString(42));
```

The generator infers whether the second argument is a `char` or `string` and calls `__concat_char` or `__concat_str` accordingly.

## substring

```rock
substring(s, from)           // from index to end
substring(s, from, end)      // inclusive range
```

```rock
string s := "Hello, World!";
string world := substring(s, 8);      // "World!"
string hello := substring(s, 1, 5);   // "Hello"
string tail := substring(s, -6, -1);  // "World!"
```

Substring indexes are 1-based by default and negative indexes count back from the end. `set_string_index_base(0)` changes substring indexing to zero-based at runtime. The three-argument form passes `start` and inclusive `end` through to `__substring_range()`.

## toString

```rock
toString(n)   // works for int, byte, word, dword, float
```

```rock
string s := toString(42);            // "42"
string b := toString(to_byte(255));  // "255"
```

## charAt / setCharAt

```rock
charAt(s, index)          // returns char at index
setCharAt(s, index, c)    // mutate char at index
```

```rock
char c := charAt("hello", 0);   // 'h'
```

## length

```rock
length(s)   // returns int length (also works for arrays)
```

`length()` uses `_Generic` on the host target to dispatch to string or array length. On SDCC builds, `length` falls back to array length only.

## print / printf

```rock
print(s)          // print a Rock string
printf("fmt %d", n)  // C-style format print
```

`print` writes each character in the Rock string and flushes stdout. `printf` accepts one expression in Rock source: string expressions are emitted as `printf("%s", string_to_cstr(expr))`, while non-string expressions are forwarded to C's `printf`.

## C String Interop

`string_to_cstr`, `cstr_to_string`, and `new_string` exist in the runtime for C interop and explicit string copying. `string_to_cstr` is used by the generator when a Rock string expression has to be passed to C `printf`. `cstr_to_string` and `new_string` are C out-parameter helpers today; they are runtime support functions rather than polished Rock expression syntax.

## Char Type

```rock
char c := 'A';
```

Single characters use single quotes. Concatenating a char with a string uses `__concat_char()`.

See [[syntax/types]] for type declarations, [[syntax/arrays]] for string arrays, [[syntax/builtins-and-io]] for the complete built-in list, and [[concepts/string-representation]] for the runtime struct.
