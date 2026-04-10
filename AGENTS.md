# Repository Guidelines

## Project Structure & Module Organization
`src/` contains compiler components (lexer, parser, typechecker, generator) and `src/lib/` holds the runtime plus ZX Next interop linked into generated programs. `build/` only caches objects created by `make` and should stay out of commits. Root-level binaries include `rockc` and the `rock` driver; language notes live in `docs/`, while `test/` collects `.rkr` regression inputs that define current behavior.

## Build, Test, and Development Commands
- `make` — compiles everything under `src/` into `rockc` using `gcc -Werror -Wall -Wextra`. Run it before testing or submitting patches.
- `make clean` — removes `build/` and `rockc`.
- `./rock path/to/foo.rkr [foo.exe]` — transpiles via `rockc` then builds the executable. Pass `--target=gcc` (default) for native runs or `--target=zxn` for ZX Next binaries (requires the `zcc +zxn` toolchain).
- `./run_tests.sh [test/substring_test.rkr]` — compiles each `.rkr` with `./rock` and runs the resulting binaries; provide a path to iterate on a single test.

## Coding Style & Naming Conventions
All C sources use two-space indentation and brace-on-same-line style (see `src/parser.c`). Use `snake_case` for functions, locals, and struct fields, reserving `UPPER_SNAKE_CASE` for enums or macros. Prefer `static` helpers for module-internal behavior and include only the headers a file needs. When invoking `./rock --debug`, generated C is formatted with `clang-format`; keep manual changes similarly tidy.

## Testing Guidelines
Tests live in `test/` with descriptive names such as `array_test.rkr` or `module_decl_test.rkr` so `run_tests.sh` can auto-discover them. Each file should import `test/Assert.rkr` helpers and emit `PASS`/`FAIL` strings that the harness parses. Run the suite after touching parsing, code generation, or runtime code. Runtime additions in `src/lib/` usually need a new `.rkr` plus, when relevant, a ZX Next check via `./rock --target=zxn`.

## Commit & Pull Request Guidelines
Existing commits are short and imperative (e.g., “Fix relative path resolution”), so follow that voice and keep unrelated work split. Before opening a PR, verify `make`, `./run_tests.sh`, and representative `./rock` invocations for each target you touched. PR descriptions should summarize behavioral impact, mention docs/tests updates, link issues, and attach logs or ZX screenshots when they illustrate new output.
