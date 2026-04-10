---
title: Syntax Index
category: syntax
tags: [syntax, index, keywords]
sources: []
updated: 2026-04-09
status: current
---

# Syntax Index

Quick reference index for Rock language syntax. Each topic has a dedicated page.

## Type System
→ [[syntax/types]] — scalar types, arrays, string, char, boolean, type-first declarations

## Variables and Binding
→ [[syntax/types]] — `let`, `dim`, type-first style, default values

## Functions and Methods
→ [[syntax/functions-and-methods]] — `sub`, parameter lists, return types, method syntax

## Control Flow
→ [[syntax/control-flow]] — `if/then/else`, `while`, `for` counter loop, `for x in` iterator, `match`

## Arrays
→ [[syntax/arrays]] — declaration, `append`, `get`, `set`, `pop`, `insert`, `length`, iteration

## Records, Enums, Modules
→ [[syntax/modules-and-records]] — `record`, `pro`, `enum`, `module`

## String Operations
→ [[syntax/strings]] — `concat`, `substring`, `to_string`, `get_nth_char`, `set_nth_char`, `get_string_length`

## Embed Blocks
→ [[syntax/embed]] — `@embed c … @end c`, `@embed asm … @end asm`

## Memory Access
→ [[syntax/control-flow]] (see also [[targets/zxn-z80]]) — `peek`, `poke`

## Include System
→ [[parser/parser-overview]] — `include "file.rkr"`, `module Name;`

---

## Keyword Quick Reference

| Keyword | Purpose |
|---------|---------|
| `sub` | Function / method definition |
| `let` | Immutable variable binding |
| `dim` | Mutable variable declaration |
| `if` / `then` / `else` | Conditional |
| `while` / `do` | While loop |
| `for` / `to` / `in` | Counter loop / iterator loop |
| `iter` | Iterator loop (alias for `for x in`) |
| `match` | Pattern match |
| `return` | Return from function |
| `record` | Composite value type |
| `pro` | Product (tagged union) type |
| `enum` | Enumeration type |
| `module` | Named singleton type |
| `include` | Import another `.rkr` file |
| `@embed` / `@end` | Inline C or Z80 assembly |

## Built-in Function Quick Reference

| Function | Description |
|----------|-------------|
| `append(arr, val)` | Add element to end of array |
| `get(arr, idx)` | Get element at index |
| `set(arr, idx, val)` | Set element at index |
| `pop(arr)` | Remove and return last element |
| `insert(arr, idx, val)` | Insert element at index, shifting right |
| `length(arr\|str)` | Length of array or string |
| `concat(s1, s2)` | Concatenate two strings (or string+char) |
| `substring(s, from)` | Substring from index to end |
| `substring(s, from, len)` | Substring of given length |
| `to_string(n)` | Convert number to string |
| `get_nth_char(s, i)` | Get character at index |
| `set_nth_char(s, i, c)` | Set character at index |
| `get_string_length(s)` | String length |
| `print(s)` | Print string |
| `printf(fmt)` | C-style printf |
| `peek(addr)` | Read byte from memory address |
| `poke(addr, val)` | Write byte to memory address |
| `get_args()` | Get command-line arguments as `string[]` |
