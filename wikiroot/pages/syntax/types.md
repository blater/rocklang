---
title: Types and Variable Declarations
category: syntax
tags: [types, variables, declaration, default-values, globals]
sources: []
updated: 2026-04-27
status: current
---

# Types and Variable Declarations

## Scalar Types

| Rock type | C equivalent | Notes |
|-----------|-------------|-------|
| `int` | `int` | Platform-width signed integer |
| `byte` | `uint8_t` | Unsigned 8-bit |
| `word` | `uint16_t` | Unsigned 16-bit |
| `dword` | `uint32_t` | Unsigned 32-bit |
| `string` | `string` | Struct: `{ char *data; size_t length; }` |
| `char` | `char` | Single character |
| `boolean` | `char` | `true` / `false` macros from `typedefs.h` |
| `void` | `void` | Return type only |

## Array Types

```rock
int[]        // dynamic array of int (grows as needed)
string[10]   // fixed-size array of 10 strings
```

All arrays are represented as `__internal_dynamic_array_t` at runtime. Fixed-size arrays set `max_capacity`; attempting to exceed it halts the program.

## Variable Declarations

Rock variable declarations use type-first syntax:

```rock
int x := 10;
string s;
int[] arr;
Point p := { x := 3, y := 4 };
```

The variable name follows the type. `:=` introduces an initial value; omitting it requests a type-appropriate default.

There is no active `let` or `dim` keyword in the lexer/parser. Older notes that describe `let name: type` or `dim name: type` forms are stale for the current compiler.

## Global Variables

The same declaration syntax works at file scope:

```rock
int TRUE := 1;
string[] names;
```

Scalars can be emitted directly as C globals. Array and module globals need runtime allocation, so the generator emits a `NULL` global and defers the real initialisation into `main()`.

## Default Initialisation Values

When a variable is declared without an explicit initial value, it receives a type-appropriate default:

| Type | Default |
|------|---------|
| `int`, `byte`, `word`, `dword` | `0` |
| `string` | `""` (empty string) |
| `char` | `'\0'` |
| `boolean` | `false` |
| `Type[]` | empty dynamic array |
| User-defined `record` | **No default** — must be explicitly initialised |

## Type Conversions

Built-in conversion functions:

```rock
int n := to_int(some_byte);
byte b := to_byte(some_int);
word w := to_word(some_int);
dword d := to_dword(some_int);
string s := toString(42);       // "42"
```

## Literals

```rock
42          // integer literal
"hello"     // string literal
'A'         // char literal
true        // boolean
false       // boolean
NULL        // C null sentinel; not a dedicated Rock keyword
```

Numeric literals are decimal digits only; hexadecimal forms such as `0xff` are not currently tokenised as one number. `true` and `false` are C macros from `src/lib/typedefs.h`, not lexer keywords. `NULL` is parsed as an identifier and emitted as C `NULL`, so it is mainly useful when deliberately passing a null sentinel through to generated C.

## Type Annotations in Sub Parameters

```rock
sub add(int a, int b) returns int {
  return a + b;
}
```

All parameters require explicit type annotations using **type-first** syntax (`type name`). Return type, when present, is introduced by the `returns` keyword after the parameter list. If omitted, return type defaults to `void`.

See [[syntax/functions-and-methods]] for full function syntax, [[syntax/arrays]] for array operations, and [[syntax/modules-and-records]] for composite types.
