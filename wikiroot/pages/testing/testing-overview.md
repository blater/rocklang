---
title: Testing Overview
category: testing
tags: [testing, test-suite, assertions, rkr]
sources: []
updated: 2026-04-09
status: current
---

# Testing Overview

Rock uses a file-based test suite. Each test is a `.rkr` Rock source file in `test/`. Tests are driven by a shell script (`run_tests.sh`) that compiles each file, runs it, and checks the exit code.

## Test Harness

`test/Assert.rkr` provides the assertion helper used by all tests:

```rock
include "test/Assert.rkr"
```

`Assert.rkr` defines:
- `assertTrue(cond: boolean, msg: string)` — print pass/fail, increment counters
- `assertEqual(a, b, msg: string)` — various overloads for int, string, char, byte, word, dword
- `printResults()` — print final pass/fail counts; exits non-zero if any assertion failed

Tests include `Assert.rkr`, call assertion functions in named test sub-functions, call `printResults()` in `main`, and exit with failure if any assertion failed.

## Running Tests

```bash
./run_tests.sh                    # Run all tests (host/gcc target only)
./run_tests.sh test/foo.rkr       # Run a single test file
```

The script uses `./rock` to compile each test, runs the resulting binary, and reports PASS / FAIL per test. ZXN tests must be compiled and run manually — `run_tests.sh` has no `--target` option.

## Test Suite (27 tests)

| Test file | Features covered |
|-----------|-----------------|
| `simple_test.rkr` | Basic variables, string assignment, assertions |
| `array_test.rkr` | Dynamic arrays, `append`, `get`, `length`, fixed-size arrays |
| `array_insert_test.rkr` | `insert()` at beginning, middle, end; multiple sequential inserts |
| `assign_test.rkr` | Variable assignment, reassignment |
| `assignment_default_test.rkr` | Default initialisation for all types (`dim x: int` → `0`, etc.) |
| `byte_test.rkr` | `byte` type arithmetic and casting |
| `byte_advanced_test.rkr` | Advanced byte operations, overflow |
| `word_test.rkr` | `word` type (uint16) |
| `dword_test.rkr` | `dword` type (uint32) |
| `casting_test.rkr` | `to_int`, `to_byte`, `to_word`, `to_dword` conversions |
| `concat_test.rkr` | String concatenation with `concat()`, char+string |
| `substring_test.rkr` | `substring(s, from)` — basic substring |
| `substring_advanced_test.rkr` | `substring(s, from, len)` — ranged substring |
| `tostring_test.rkr` | `to_string(n)` for numeric types |
| `format_test.rkr` | `printf` format string output |
| `enum_test.rkr` | Enum declarations, enum values in expressions |
| `loop_test.rkr` | Counter loops (`for i := 0 to N`) |
| `for_each_test.rkr` | Iterator loops (`for x in arr`) |
| `memory_test.rkr` | Dynamic allocation, reference semantics |
| `peek_poke_test.rkr` | `peek()` / `poke()` memory access |
| `peek_poke_simple_test.rkr` | Simplified peek/poke |
| `peek_poke_minimal.rkr` | Minimal peek/poke |
| `peek_poke_silent.rkr` | peek/poke without assertion output |
| `unary_test.rkr` | Unary minus, unary not |
| `embed_test.rkr` | `@embed c … @end c` inline C blocks |
| `embed_simple_test.rkr` | Simple embed block |
| `embed_inline_test.rkr` | Inline embed usage |
| `get_args_test.rkr` | `get_args()` — command-line argument access |

## Test Status (as of 2026-04-09)

- **Host (gcc):** 27/27 passing
- **ZXN:** Most tests pass; `enum_test.rkr` fails due to SDCC enum syntax incompatibility (pre-existing, not a regression)

## Adding a New Test

1. Create `test/my_test.rkr`
2. `include "test/Assert.rkr"`
3. Write test sub-functions calling `assert*` helpers
4. Call test functions + `printResults()` from `main`
5. Add to `run_tests.sh` test list

## Assert.rkr Pattern

```rock
include "test/Assert.rkr"

sub testAddition(): void {
  let result: int := 1 + 2;
  assertEqual(result, 3, "1 + 2 should be 3");
}

sub main(): void {
  testAddition();
  printResults();
}
```

See [[overview]] for the compilation pipeline and [[targets/host-gcc]] / [[targets/zxn-z80]] for platform-specific notes.
