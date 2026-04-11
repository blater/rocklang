---
title: Testing Overview
category: testing
tags: [testing, test-suite, assertions, rkr]
sources: []
updated: 2026-04-11
status: current
---

# Testing Overview

Rock uses a file-based test suite. Each test is a `.rkr` Rock source file in `test/`. Tests are driven by a shell script (`run_tests.sh`) that compiles each file, runs it, and checks the exit code.

## Test Harness

`test/Assert.rkr` provides the assertion helper used by most tests:

```rock
include "Assert.rkr"
```

`Assert.rkr` defines:
- `AssertTrue(desc, value)` — print pass/fail for a truthy condition
- `AssertEQ(desc, expected, actual)` — integer equality assertion
- `AssertEquals(desc, expected, actual)` — string equality assertion

Tests print `PASS:` / `FAIL:` lines directly, and `run_tests.sh` parses those markers from program output.

## Running Tests

```bash
./run_tests.sh                    # Run all tests (host/gcc target only)
./run_tests.sh test/foo.rkr       # Run a single test file
```

The script uses `./rock` to compile each test, runs the resulting binary, and reports PASS / FAIL per test. ZXN tests must be compiled and run manually — `run_tests.sh` has no `--target` option.

## Test Suite (39 auto-discovered tests)

| Test file | Features covered |
|-----------|-----------------|
| `simple_test.rkr` | Basic variables, string assignment, assertions |
| `assign_test.rkr` | Variable assignment, reassignment |
| `var_declare_test.rkr` | Global variables, default-initialised scalar declarations |
| `assignment_default_test.rkr` | Default initialisation for all types (`int x;` → `0`, etc.) |
| `new_style_decl_test.rkr` | New-style (C/Java-like) variable declarations and record instantiation |
| `new_style_params_test.rkr` | New-style function parameters (`type name` syntax) |
| `unary_test.rkr` | Unary minus, unary not |
| `casting_test.rkr` | `to_int`, `to_byte`, `to_word`, `to_dword` conversions |
| `byte_test.rkr` | `byte` type arithmetic and casting |
| `byte_advanced_test.rkr` | Advanced byte operations, overflow |
| `word_test.rkr` | `word` type (uint16) |
| `dword_test.rkr` | `dword` type (uint32) |
| `array_test.rkr` | Dynamic arrays, `append`, `get`, `length`, fixed-size arrays |
| `array_insert_test.rkr` | `insert()` at beginning, middle, end; multiple sequential inserts |
| `array_field_expr_receiver_test.rkr` | Field access on postfix expression receivers such as `make_wrapper().Names` |
| `array_field_direct_index_test.rkr` | Direct indexing and assignment through record array fields |
| `concat_test.rkr` | String concatenation with `concat()`, char+string |
| `substring_test.rkr` | `substring(s, from)` — basic substring |
| `substring_advanced_test.rkr` | `substring(s, from, len)` — ranged substring |
| `tostring_test.rkr` | `to_string(n)` for numeric types |
| `format_test.rkr` | `printf` format string output |
| `record_init_test.rkr` | Record instantiation, field access, field mutation |
| `record_field_test.rkr` | Record fields holding arrays; array ops through record references |
| `methods.rkr` | Method syntax (`sub Type.method()`), `this` binding, return values |
| `module_decl_test.rkr` | Module declaration, method dispatch, singleton state |
| `module_instance_test.rkr` | Multiple module instances, state isolation |
| `enum_test.rkr` | Enum declarations, enum values in expressions |
| `loop_test.rkr` | Counter loops (`for i := 0 to N`) |
| `for_each_test.rkr` | Iterator loops (`for x in arr`) |
| `memory_test.rkr` | Dynamic allocation, reference semantics |
| `peek_poke_test.rkr` | `peek()` / `poke()` memory access |
| `peek_poke_simple_test.rkr` | Simplified peek/poke |
| `peek_poke_minimal.rkr` | Minimal peek/poke |
| `peek_poke_silent.rkr` | peek/poke without assertion output |
| `embed_test.rkr` | `@embed c … @end c` top-level inline C function |
| `embed_simple_test.rkr` | Minimal top-level embed usage |
| `embed_inline_test.rkr` | `@embed c` inside a function body modifying a local variable |
| `embed_asm_test.rkr` | `@embed asm` inside a function; verifies compile-clean on host |
| `get_args_test.rkr` | `get_args()` — command-line argument access |

## Test Status (as of 2026-04-11)

- **Host (gcc):** 39/39 auto-discovered tests passing
- **ZXN:** Most tests pass; `enum_test.rkr` currently fails due to SDCC enum syntax incompatibility

## Adding a New Test

1. Create `test/my_test.rkr`
2. `include "Assert.rkr"` for top-level tests under `test/`
3. Write test code that prints `PASS:` / `FAIL:` via the assertion helpers
4. Run it with `./run_tests.sh test/my_test.rkr`
5. Put the file directly under `test/` if you want auto-discovery

## Assert.rkr Pattern

```rock
include "Assert.rkr"

sub main(): void {
  Assert a;
  int result := 1 + 2;
  a.AssertEQ("1 + 2 should be 3", 3, result);
}
```

See [[overview]] for the compilation pipeline and [[targets/host-gcc]] / [[targets/zxn-z80]] for platform-specific notes.
