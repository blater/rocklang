---
title: Host Target (gcc)
category: targets
tags: [host, gcc, linux, macos, target]
sources: []
updated: 2026-04-10
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

The generator emits **relative runtime header names**:
```c
#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"
```

The `rock` driver invokes `gcc` with `-I "$ROCK_ROOT/src/lib"` so those headers resolve against the runtime library directory. Host builds no longer depend on absolute repository paths embedded into generated C.

## Compilation Command

```bash
gcc -Wall -Wno-unused-variable \
    -I "$ROCK_ROOT/src/lib" \
    out.c \
    "$ROCK_ROOT/src/lib/alloc.c" \
    "$ROCK_ROOT/src/lib/fundefs.c" \
    "$ROCK_ROOT/src/lib/fundefs_internal.c" \
    "$ROCK_ROOT/src/lib/asm_interop.c" \
    -o out
```

## Runtime Library

All host programs link against:
- `src/lib/fundefs_internal.c` — array ops, string helpers, arg access
- `src/lib/fundefs.c` — public runtime helpers and declarations used by generated code
- `src/lib/alloc.c` — arena allocator helpers used by the runtime
- `src/lib/asm_interop.c` — host stub layer for shared runtime entry points

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

See [[testing/testing-overview]] for the current auto-discovered host test count and status.
