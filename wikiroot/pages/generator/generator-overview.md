---
title: Generator Overview
category: generator
tags: [generator, code-generation, transpile, c-output, pre_f, string-temporaries, array-wrappers]
sources: []
updated: 2026-04-10
status: current
---

# Generator Overview

The **generator** walks the AST produced by the parser and emits a C source file. Implemented in `src/generator.c`, `src/generator.h`. This is the largest and most complex phase of the compiler.

## Data Structures

### generator_t
```c
typedef struct generator_t {
  FILE            *f;                          // Primary output file
  name_table_t     table;                      // Scoped symbol table
  int              str_tmp_counter;            // Counter for __strtmp_N vars
  target_t         target;                     // TARGET_HOST or TARGET_ZXN
  FILE            *pre_f;                      // Secondary buffer (statement setup)
  char            *pre_buf;                    // Backing memory for pre_f
  size_t           pre_buf_size;
  string_view      current_module_type;        // Set while generating a module method
  int              in_global_scope;
  ast_array_t      deferred_module_inits;      // Module instances needing _new() calls
  char           **deferred_global_init_code;  // Deferred global init code strings
  int              deferred_global_init_count;
  int              deferred_global_init_capacity;
} generator_t;
```

## Key Functions

| Function | Responsibility |
|----------|---------------|
| `transpile(generator_t*, ast_t)` | Entry point; emits file header (includes, pragmas) then walks program nodes |
| `generate_statement(g, node)` | Dispatch for statement nodes |
| `generate_expression(g, node)` | Dispatch for expression nodes; writes to `g->f` |
| `generate_funcall(g, node)` | Handles both builtins and user-defined calls |
| `generate_method_call(g, node)` | Emits mangled method call |
| `generate_array_op(g, …)` | Unified handler for `append`, `get`, `set`, `pop`, `insert` |
| `generate_fundef(g, node)` | Emit C function signature + body |
| `generate_vardef(g, node)` | Emit variable declaration with type conversion |
| `generate_loop(g, node)` | Emit `for(int i = start; i <= end; i++)` |
| `generate_iter_loop(g, node)` | Emit element-iterating for-loop over array |
| `generate_type(g, type_node)` | Map Rock type to C type name |
| `capture_expression(g, node)` | Evaluate expression into a `char*` buffer (used for deferred contexts) |
| `emit_concat(g, node)` | Type-aware string concatenation setup |
| `emit_substring(g, node)` | Emit substring extraction |
| `emit_string_literal(g, tok)` | Emit `__rock_make_string(…)` call and temp variable setup |
| `allocate_string_tmp(g)` | Return next `__strtmp_N` name, increment counter |
| `infer_expr_type(g, node)` | Best-effort type inference from AST + name table |
| `get_array_element_type(g, node)` | Infer element type of an array expression |
| `register_builtin(table, name, type)` | Pre-register builtin functions in name table |

## Output Model

### Primary buffer (f)
The main C output file. Normal C statements and expressions are written here.

### Pre-buffer (pre_f)
A memory-backed `FILE*` that accumulates setup code that must precede the current statement. At the start of each statement, `pre_f` is flushed to `f` before the statement itself is emitted.

**Why needed:** In C, variable declarations cannot appear inside expression context. String literal setup and string temporaries require statements before the expression using them is valid. This is especially important for ZXN/SDCC.

**Pattern:**
```
// For:  print(concat(a, b));
// pre_f receives:
  rock_string __strtmp_0 = __concat_str(a, b);
// f receives:
  print(__strtmp_0);
```

## Include Emission

At the top of every generated C file, `transpile()` emits runtime includes:

**All targets** (relative runtime header names):
```c
#include "alloc.h"
#include "fundefs.h"
#include "fundefs_internal.h"
#include "typedefs.h"
```

The compiler driver (`rock`) passes `-I "$ROCK_ROOT/src/lib"` for both host and ZXN builds so these includes resolve against the runtime library directory.

## Type Mapping

| Rock type | C type |
|-----------|--------|
| `int` | `int` |
| `byte` | `uint8_t` |
| `word` | `uint16_t` |
| `dword` | `uint32_t` |
| `string` | `rock_string` (struct with `.data` + `.length`) |
| `char` | `char` |
| `boolean` | `int` |
| `Type[]` | `__internal_dynamic_array_t` |
| `Type[N]` | `__internal_dynamic_array_t` (max_capacity set) |
| `void` | `void` |

## Array Wrapper Synthesis

For each element type `T` encountered in array operations, the generator automatically emits wrapper functions at the top of the output file:

```c
__internal_dynamic_array_t T_make_array(void) { ... }
void T_push_array(__internal_dynamic_array_t *arr, T elem) { ... }
T    T_pop_array(__internal_dynamic_array_t *arr) { ... }
T    T_get_elem(__internal_dynamic_array_t *arr, int idx) { ... }
void T_set_elem(__internal_dynamic_array_t *arr, int idx, T elem) { ... }
void T_insert(__internal_dynamic_array_t *arr, int idx, T elem) { ... }
```

Element type is inferred from the array expression via `get_array_element_type()` and `infer_expr_type()`. This includes field-derived array expressions such as `a.Items`, nested paths such as `h.Data.Names`, and postfix receivers such as `make_wrapper().Names`.

## Method Call Mangling

`sub Type.method(…)` → emitted as `TypeName_methodName(this, …)`  
`sub Type[].method(…)` → emitted as `TypeName_array_methodName(this, …)`

During a method call, `infer_expr_type()` determines the receiver's type from the name table, then the generator constructs the mangled name.

## Builtin Functions

The generator recognises these names in `generate_funcall()` and routes them to specialised handlers:

| Rock call | Handler | Notes |
|-----------|---------|-------|
| `append(arr, val)` | `generate_array_op` | `T_push_array` |
| `get(arr, idx)` | `generate_array_op` | `T_get_elem` |
| `set(arr, idx, val)` | `generate_array_op` | `T_set_elem` |
| `pop(arr)` | `generate_array_op` | `T_pop_array` |
| `insert(arr, idx, val)` | `generate_array_op` | `T_insert` |
| `length(arr\|str)` | inline | `__length_array` or `__length_string` |
| `concat(s1, s2)` | `emit_concat` | type-inferred char vs string |
| `substring(s, from[, len])` | `emit_substring` | 2 or 3 arg forms |
| `to_string(n)` | inline | `__to_string_int` etc. |
| `print(s)` | inline | `printf("%s", s.data)` |
| `printf(fmt)` | inline | forwarded to C `printf` |
| `peek(addr)` | inline | memory-mapped byte read |
| `poke(addr, val)` | inline | memory-mapped byte write |
| `get_args()` | inline | `fill_cmd_args()` |

## Module Initialisation

When a module instance variable is declared (e.g. `GameState state;`), the generator defers emitting the `GameState_new()` initialisation call until the synthesised `main()` function, ensuring all global state is initialised before program code runs.

## ZXN-Specific Behaviour

- `pre_f` buffer is used more aggressively to split statements that SDCC cannot handle inline.
- `zpragma_zxn.inc` is included at the top of the generated C file (memory bank layout).
- Z80 assembly embed blocks (`@embed asm … @end asm`) are passed through verbatim.

See [[targets/zxn-z80]] for the full ZXN compilation story, [[concepts/name-table]] for symbol resolution, and [[concepts/string-representation]] and [[concepts/array-internals]] for runtime structures.
