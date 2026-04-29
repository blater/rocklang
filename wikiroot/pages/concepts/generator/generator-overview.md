---
title: Generator Overview
category: generator
tags: [generator, code-generation, transpile, c-output, pre_f, string-temporaries, array-wrappers]
sources: []
updated: 2026-04-25
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
  ast_t            program;                    // Top-level program node (set at top of transpile(); used for arity-based overloading)
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
| `float` | `float` |
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

## Function Overloading (arity-based)

Rock supports **arity-based overloading** for top-level user functions: two or more `sub name(...)` declarations may share a name as long as they take a different number of arguments. Dispatch happens in the generator (there is no pre-pass typechecker) and is strictly arity-keyed — parameter types are not considered. Two overloads at the same arity are a user error and will collide at C compile time.

### Mangling rule

- If a Rock name has **exactly one** user fundef, the C symbol is the bare name (unchanged from the pre-overload generator — no diff churn for existing code).
- If a Rock name has **two or more** user fundefs, every occurrence mangles to `name__N` where `N` is the arity. Both the forward declaration, the definition, and every call site run through the same `emit_fun_name()` helper, so mangling is consistent across the whole translation unit.

Examples (from `test/overload_test.rkr`):

```rock
sub greet() returns string                  { return "hello, stranger"; }
sub greet(string name) returns string       { return concat("hello, ", name); }
sub add(int a, int b) returns int           { return a + b; }
sub add(int a, int b, int c) returns int    { return a + b + c; }
```

generates:

```c
string greet__0(void);
string greet__1(string name);
int    add__2(int a, int b);
int    add__3(int a, int b, int c);
```

### Scope and limitations

- **User fundefs only.** Builtins are not counted as overloads in Phase 1 — a user `fun print(...)` still shadows the builtin `print` exactly as it did before.
- **Non-method only.** Methods (`sub Type.method`) already go through their own `mangle_method()` path and are unaffected.
- **Return-type inference.** `infer_expr_type` returns the last-seen name-table entry for a given name, so if two overloads disagree on return type the inferred type may be wrong at some call sites. In practice most overloads share a return type; a full type-based pass is the Phase 3 escape hatch.
- **Hardcoded builtin fast-paths** (`append`, `get`, `set`, `pop`, `insert`, `substring`, `concat`, `toString`, `printf`) are matched by lexeme at the top of `generate_funcall` and never reach the mangling helper — their names are effectively reserved.

## Method Call Mangling

`sub Type.method(…)` → emitted as `TypeName_methodName(this, …)`  
`sub Type[].method(…)` → emitted as `TypeName_array_methodName(this, …)`

During a method call, `infer_expr_type()` determines the receiver's type from the name table, then the generator constructs the mangled name.

## Builtin Functions

The generator recognises these names in `generate_funcall()` and routes them to specialised handlers. The table is organised by category.

### Array operations
| Rock call | Notes |
|-----------|-------|
| `append(arr, val)` | `T_push_array` |
| `get(arr, idx)` | `T_get_elem` |
| `set(arr, idx, val)` | `T_set_elem` |
| `pop(arr)` | `T_pop_array` |
| `insert(arr, idx, val)` | `T_insert` |
| `length(arr\|str)` | `__length_array` or `__length_string` |

### String operations
| Rock call | Notes |
|-----------|-------|
| `concat(s1, s2)` | `emit_concat`; type-inferred char vs string |
| `substring(s, from[, len])` | `emit_substring`; 2 or 3 arg forms |
| `toString(n)` | `__to_string_int` etc. |
| `charAt(s, i)` | character at index |
| `setCharAt(s, i, c)` | mutate character at index |
| `equals(a, b)` | string equality predicate |
| `new_string(cstr)` | construct `rock_string` from C string literal |
| `cstr_to_string(cstr)` | alias |
| `string_to_cstr(s)` | extract `.data` pointer |

### I/O and output
| Rock call | Notes |
|-----------|-------|
| `print(s)` | `printf("%s", s.data)` |
| `print(x, y, s)` | positioned text via `print_at(x, y, s)` (3-arg overload) |
| `printf(fmt, ...)` | forwarded to C `printf` |
| `putchar(c)` | single character output |
| `get_args()` | `fill_cmd_args()` |
| `read_file(path)` | read file to string |
| `write_string_to_file(path, s)` | write string to file |
| `get_abs_path(path)` | resolve to absolute path |

### Type conversions
| Rock call | Notes |
|-----------|-------|
| `to_int(x)` | cast to `int` |
| `to_byte(x)` | cast to `uint8_t` |
| `to_word(x)` | cast to `uint16_t` |
| `to_dword(x)` | cast to `uint32_t` |
| `to_float(x)` | cast to `float` |

### Memory access
| Rock call | Notes |
|-----------|-------|
| `peek(addr)` | memory-mapped byte read |
| `poke(addr, val)` | memory-mapped byte write |

### Display — text and attributes
| Rock call | Notes |
|-----------|-------|
| `cls()` | clear screen |
| `ink(colour)` | set foreground colour attribute |
| `paper(colour)` | set background colour attribute |
| `bright(on)` | set bright attribute |
| `flash(on)` | set flash attribute |
| `inverse(on)` | set inverse attribute |
| `over(mode)` | set draw mode: 0 = OR (default), 1 = XOR |

### Display — raster graphics
| Rock call | Notes |
|-----------|-------|
| `plot(x, y)` | set single pixel |
| `point(x, y)` | read pixel from shadow buffer |
| `draw(x0, y0, x1, y1)` | line with H/V fast paths + Bresenham |
| `polyline(xs, ys)` | connected segments via `draw()` |
| `circle(cx, cy, r)` | outline via midpoint algorithm |
| `fill(x0, y0, x1, y1)` | axis-aligned filled rectangle |
| `triangle(x1,y1, x2,y2, x3,y3)` | triangle outline (three `draw()` calls) |
| `trianglefill(x1,y1, x2,y2, x3,y3)` | scanline-rasterised solid fill |

### Border and hardware
| Rock call | Notes |
|-----------|-------|
| `border(colour)` | set border colour via port `$FE` |
| `border_get()` | read shadowed border colour |
| `next_reg_set(reg, val)` | write ZXN Next Register |
| `next_reg_get(reg)` | read ZXN Next Register |
| `cpu_speed_set(speed)` | set ZXN CPU speed (0–3) |
| `cpu_speed_get()` | read current CPU speed |
| `mmu_set(slot, bank)` | configure MMU bank slot |

### Keyboard
| Rock call | Notes |
|-----------|-------|
| `scan_keyboard()` | read hardware matrix into buffer |
| `key_pressed(key)` | test if key is held (action game use) |
| `inkey()` | ASCII key via ROM LAST_K (menu/text use) |
| `keypress()` | wait for key release |

### Timing and audio
| Rock call | Notes |
|-----------|-------|
| `sleep(ms)` | delay in milliseconds |
| `beep(dur, pitch)` | ZXN beep via ROM or host bell |

### Random numbers
| Rock call | Notes |
|-----------|-------|
| `randomize(seed)` | seed the RNG |
| `random_byte()` | random `byte` value |
| `random_word()` | random `word` value |

### Helpers / bit ops
| Rock call | Notes |
|-----------|-------|
| `odd(n)` | 1 if `n` is odd |
| `even(n)` | 1 if `n` is even |
| `hi(w)` | high byte of a `word` |
| `lo(w)` | low byte of a `word` |
| `swap(w)` | byte-swap a `word` |
| `upcase(c)` | uppercase a `char` |
| `locase(c)` | lowercase a `char` |
| `abs_int(n)` | absolute value of `int` |
| `abs_word(n)` | absolute value of `word` |

### Math (float)
| Rock call | Notes |
|-----------|-------|
| `fsin(x)` | sine |
| `fcos(x)` | cosine |
| `fsqrt(x)` | square root |
| `fabs_float(x)` | absolute value |
| `fpi()` | π constant |

### Control
| Rock call | Notes |
|-----------|-------|
| `exit(code)` | terminate process |
| `halt(code)` | terminate (ZXN: halt instruction; host: `exit`) |

## Module Initialisation

When a module instance variable is declared (e.g. `GameState state;`), the generator defers emitting the `GameState_new()` initialisation call until the synthesised `main()` function, ensuring all global state is initialised before program code runs.

## ZXN-Specific Behaviour

- `pre_f` buffer is used more aggressively to split statements that SDCC cannot handle inline.
- `zpragma_zxn.inc` is included at the top of the generated C file (memory bank layout).
- `@embed asm` blocks are wrapped in `#ifdef __SDCC` / `#asm` / `#endasm` / `#endif` — Z88DK's `zcc` processes `#asm`/`#endasm` before SDCC sees the file; gcc silently skips the block via the preprocessor guard. See [[syntax/embed]].

See [[targets/zxn-z80]] for the full ZXN compilation story, [[concepts/name-table]] for symbol resolution, and [[concepts/string-representation]] and [[concepts/array-internals]] for runtime structures.
