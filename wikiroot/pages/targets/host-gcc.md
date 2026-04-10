---
title: Host Target (gcc)
category: targets
tags: [host, gcc, linux, macos, target]
sources: []
updated: 2026-04-09
status: current
---

# Host Target (gcc)

The **host target** compiles Rock programs to native binaries on Linux or macOS using `gcc`. It is the default target (no flag needed).

## Toolchain

| Tool | Purpose |
|------|---------|
| `rock` | Rock → C transpiler |
| `gcc` | C → native binary |
| `rockc` | Shell script driving both steps |

## Invocation

`rock` is the full build script (transpile + compile). `rockc` is the transpiler binary alone (Rock → C).

```bash
# Full build (transpile + compile):
rock input.rkr [output.exe] [--target=gcc] [--debug]

# Transpile only (Rock → C):
rockc input.rkr [output.c] [--target=gcc]
```

Default output name is `<basename>.exe` (e.g. `test/foo.rkr` → `test/foo.exe`).  
`--debug` keeps the generated `.c` file on disk and runs `clang-format` on it — useful for inspecting generated code.

## Include Strategy

The generator emits **absolute paths** for runtime headers:
```c
#include "/Users/blater/src/rock/src/alloc.h"
#include "/Users/blater/src/rock/src/lib/fundefs_internal.h"
```

The path is derived at runtime from `argv[0]` via `realpath()` + `dirname()`. `gcc` is passed `-I src/` so library `.c` files can `#include` each other without absolute paths.

## Compilation Command

```bash
gcc -Wall -g \
    -I "$ROCK_ROOT/src" \
    out.c \
    "$ROCK_ROOT/src/lib/fundefs_internal.c" \
    "$ROCK_ROOT/src/alloc.c" \
    -o out
```

## Runtime Library

All host programs link against:
- `src/lib/fundefs_internal.c` — array ops, string helpers, arg access
- `src/alloc.c` — arena allocator

Headers: `alloc.h`, `fundefs.h`, `fundefs_internal.h`, `typedefs.h`

## Supported Features

All Rock language features are supported on the host target:
- Dynamic and fixed-size arrays
- String operations (concat, substring, to_string)
- Records, product types, enums, modules
- Inline C embed blocks (`@embed c … @end c`)
- Include system
- peek / poke (memory access — useful for testing but not meaningful on host)

## Debugging

The `-g` flag is always passed, so `gdb` / `lldb` debugging of generated C is supported. Generated C variable and function names are derived from Rock names (with `__` prefixes for internals) making them recognisable in a debugger.

## Test Suite

All 27 tests in `test/` run on the host target. See [[testing/testing-overview]] for detail.
