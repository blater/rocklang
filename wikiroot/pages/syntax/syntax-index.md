---
title: Syntax Index
category: syntax
tags: [syntax, index, keywords, builtins]
sources: []
updated: 2026-04-28
status: current
---

# Syntax Index

Quick reference index for Rock language syntax. Each topic has a dedicated page.

## Type System
→ [[syntax/types]] — scalar types, arrays, string, char, boolean, type-first declarations

## Variables and Binding
→ [[syntax/types]] — type-first declarations, default values

## Comments and Includes
→ [[syntax/comments-and-includes]] — `//`, `/* ... */`, and `include "file.rkr"`

## Functions and Methods
→ [[syntax/functions-and-methods]] — `sub`, parameter lists, return types, method syntax

## Control Flow
→ [[syntax/control-flow]] — `if/then/else`, `while`, `for` counter loop, `for x in` iterator, `case`

## Arrays
→ [[syntax/arrays]] — declaration, `append`, `get`, `set`, `pop`, `insert`, `length`, iteration

## Records, Enums, Unions, Modules
→ [[syntax/modules-and-records]] — `record`, `enum`, `union`, `module`

## String Operations
→ [[syntax/strings]] — `concat`, `substring`, `toString`, `charAt`, `setCharAt`, `equals`, `length`

## Built-ins and I/O
→ [[syntax/builtins-and-io]] — output, file I/O, program args, conversions, `peek` / `poke`

## Embed Blocks
→ [[syntax/embed]] — `@embed c … @end c`, `@embed asm … @end asm`

## Memory Access
→ [[syntax/control-flow]] (see also [[targets/zxn-z80]]) — `peek`, `poke`

## Include System
→ [[parser-overview]] — `include "file.rkr"`, `module Name;`

---

## Keyword Quick Reference

| Keyword | Purpose |
|---------|---------|
| `sub` | Function / method definition |
| `if` / `then` / `else` | Conditional |
| `while` / `do` | While loop |
| `for` / `to` / `in` | Counter loop / iterator loop |
| `case` | Pattern match |
| `default` | Default/fallback arm in `case` |
| `return` | Return from function |
| `record` | Composite value type |
| `enum` | Enumeration of named integer constants |
| `union` | One-of-N variant type; auto-generated constructor per variant |
| `module` | Named singleton type |
| `returns` | Sub return type (e.g. `sub add(int a, int b) returns int`) |
| `include` | Import another `.rkr` file |
| `graphics` | Host graphics mode command: `graphics on;` / `graphics off;` |
| `@embed` / `@end` | Inline C or Z80 assembly |

## Built-in Function Quick Reference

| Function                        | Description                                    |
| ------------------------------- | ---------------------------------------------- |
| `append(arr, val)`              | Add element to end of array                    |
| `get(arr, idx)`                 | Get element at index                           |
| `set(arr, idx, val)`            | Set element at index                           |
| `pop(arr)`                      | Remove and return last element                 |
| `insert(arr, idx, val)`         | Insert element at index, shifting right        |
| `length(arr\|str)`              | Length of array or string                      |
| `concat(s1, s2)`                | Concatenate two strings (or string+char)       |
| `substring(s, from)`            | Substring from index to end                    |
| `substring(s, from, end)`       | Substring through inclusive end index          |
| `toString(n)`                   | Convert number to string                       |
| `charAt(s, i)`                  | Get character at index                         |
| `setCharAt(s, i, c)`            | Set character at index                         |
| `equals(a, b)`                  | String equality                                |
| `string_to_cstr(s)`             | Runtime helper for backing C string pointer    |
| `cstr_to_string(cstr)`          | Runtime out-parameter helper for C strings     |
| `new_string(s)`                 | Runtime out-parameter helper for string copies |
| `set_string_index_base(base)`   | Set substring index base                       |
| `print(s)`                      | Print string                                   |
| `printf(fmt)`                   | C-style printf                                 |
| `putchar(c)`                    | Print a single character                       |
| `exit(code)`                    | Exit program                                   |
| `read_file(path)`               | Host file read                                 |
| `write_string_to_file(s, path)` | Host file write                                |
| `get_abs_path(path)`            | Host absolute path resolution                  |
| `peek(addr)`                    | Read byte from memory address                  |
| `poke(addr, val)`               | Write byte to memory address                   |
| `get_args()`                    | Get command-line arguments as `string[]`       |
