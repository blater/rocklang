---
title: Types and Variable Declarations
category: syntax
tags: [types, variables, declaration, default-values]
sources: []
updated: 2026-04-10
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
| `string` | `rock_string` | Struct: `{ char *data; size_t length; }` |
| `char` | `char` | Single character |
| `boolean` | `int` | `true` / `false` |
| `void` | `void` | Return type only |

## Array Types

```rock
int[]        -- dynamic array of int (grows as needed)
string[10]   -- fixed-size array of 10 strings
```

All arrays are represented as `__internal_dynamic_array_t` at runtime. Fixed-size arrays set `max_capacity`; attempting to exceed it halts the program.

## Variable Declarations

Rock uses a single declaration syntax:

```rock
int x := 10;
string s;
int[] arr;
Point p := { x := 3, y := 4 };
```

The variable name follows the type. `:=` introduces an initial value; omitting it requests a type-appropriate default.

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
string s := to_string(42);      -- "42"
```

## Literals

```rock
42          -- integer literal
0xff        -- hex not currently supported (use decimal)
"hello"     -- string literal
'A'         -- char literal
true        -- boolean
false       -- boolean
null        -- null value (used with product types)
```

## Type Annotations in Sub Parameters

```rock
sub add(a: int, b: int): int {
  return a + b;
}
```

All parameters require explicit type annotations. Return type follows `:` after the parameter list. If omitted, return type defaults to `void`.

See [[syntax/functions-and-methods]] for full function syntax, [[syntax/arrays]] for array operations, and [[syntax/modules-and-records]] for composite types.
