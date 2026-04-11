---
title: Built-ins and I/O
category: syntax
tags: [builtins, io, file-io, args, memory, conversions]
sources: []
updated: 2026-04-11
status: current
---

# Built-ins and I/O

Rock's **built-in functions** are registered in `new_generator()` before user code is emitted. Some are ordinary runtime functions; others have special code generation because C needs type-specific array wrappers or temporary string setup.

## Output and Process Control

| Function | Behaviour |
|----------|-----------|
| `print(s)` | Print a Rock `string`; `NULL` strings print `NULL` |
| `printf(expr)` | One-argument bridge to C `printf`; strings are emitted as `%s` |
| `putchar(c)` | Emit one character |
| `exit(code)` | Exit the generated program |

## String Built-ins

| Function | Behaviour |
|----------|-----------|
| `concat(s1, s2)` | Concatenate strings, or append a `char` to a string |
| `substring(s, start)` | Slice from `start` to the end |
| `substring(s, start, end)` | Slice from `start` through inclusive `end` |
| `to_string(x)` / `toString(x)` | Convert `int`, `byte`, `word`, or `dword` to `string` |
| `str_eq(a, b)` | Return non-zero when two strings have equal length and bytes |
| `get_nth_char(s, index)` | Return the character at `index`, or `0` when out of range |
| `set_nth_char(s, index, c)` | Mutate a string character when `index` is in range |
| `get_string_length(s)` | Return the string length |
| `set_string_index_base(base)` | Set substring index base (`1` by default, `0` for zero-based) |
| `string_to_cstr(s)` | Runtime helper returning the backing C `char*`; used by generated `printf` paths |
| `cstr_to_string(cstr)` | Runtime out-parameter helper for wrapping a C string |
| `new_string(s)` | Runtime out-parameter helper for allocating a string copy |

See [[syntax/strings]] for examples and substring indexing detail. The out-parameter helpers are registered in the generator name table but do not yet have a polished Rock expression surface.

## Numeric Conversions

```rock
int n := to_int(to_byte(42));
byte b := to_byte(300);      // truncates to unsigned 8-bit
word w := to_word(70000);    // truncates to unsigned 16-bit
dword d := to_dword(42);
```

Host builds use C11 `_Generic` dispatch for typed conversions. SDCC builds use simple casts plus explicit helper calls for string conversion.

## Arrays and Length

| Function | Behaviour |
|----------|-----------|
| `append(arr, val)` | Push at the end |
| `insert(arr, idx, val)` | Insert at index, shifting later elements |
| `get(arr, idx)` | Return element at index |
| `set(arr, idx, val)` | Mutate element at index |
| `pop(arr)` | Remove and return the final element |
| `length(arr)` | Current array length |
| `length(s)` | String length on host via `_Generic`; array length fallback on SDCC |

See [[syntax/arrays]] for the runtime representation and fixed-size capacity rules.

## File I/O

| Function | Host target | ZXN target |
|----------|-------------|------------|
| `read_file(path)` | Reads a file into a Rock string | Returns empty string stub |
| `write_string_to_file(s, path)` | Writes string bytes to a file | No-op stub |
| `get_abs_path(path)` | Resolves with `realpath()` | Returns empty string stub |

The file I/O helpers live in `src/lib/fundefs.c`. They are host-oriented; Z80 builds provide stubs because the current ZXN runtime does not yet expose filesystem services.

## Program Arguments

```rock
string[] args := get_args();
int count := length(args);
```

`get_args()` returns command-line arguments as a `string[]`, excluding the executable name.

## Memory Access

```rock
word addr := to_word(23552);
byte val := peek(addr);
poke(addr, to_byte(42));
```

`peek` and `poke` are supplied by `asm_interop.c`. On ZXN they map to volatile memory access; on host they use a simulated 64K memory buffer.

See [[targets/zxn-z80]] for target-specific context and [[generator/generator-overview]] for how built-ins are registered.
