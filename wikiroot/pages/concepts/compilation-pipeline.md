---
title: Compilation Pipeline
category: concepts
tags: [pipeline, main, argv, lib-path, include-resolution, arena]
sources: []
updated: 2026-04-09
status: current
---

# Compilation Pipeline

The Rock transpiler is orchestrated by `src/main.c`. This page describes the full lifecycle of a compilation from CLI invocation to `.c` file output.

## CLI Invocation

```
rock <input.rkr> [output-base] [--target=zxn]
```

| Argument | Required | Default | Notes |
|----------|---------|---------|-------|
| `input.rkr` | Yes | — | Path to Rock source file |
| `output-base` | No | `"out"` | Base name; `.c` extension appended |
| `--target=zxn` | No | host | ZXN target flag |

## Step-by-Step Pipeline

### 1. Argument Parsing
`main.c` iterates `argv`, identifying input path, output base, and target flag.

### 2. Path Resolution
```c
// Resolve argv[0] to absolute binary path
char *real = realpath(argv[0], buf);
// lib_path = dirname(real) + "/src"
char *lib_path = compute_lib_path(real);
```
`lib_path` is the absolute path to the `src/` directory, computed at runtime from the binary location. This is used by the generator to emit absolute `#include` paths (host target only).

### 3. Allocator Initialisation
```c
init_compiler_stack();
```
Sets up the arena allocator. All subsequent allocations use this arena.

### 4. Lexing
```c
lexer_t lex = new_lexer(input_path);
token_array_t tokens = lex_program(&lex);
```
Reads the entire source file, produces a flat token array with an EOF sentinel.

### 5. Parsing
```c
parser_t p = new_parser(tokens, project_root);
ast_t program = parse_program(&p);
```
Consumes tokens, resolves includes (recursively lexing + splicing), builds the AST.

`project_root` is the parent directory of the input file. Include paths resolve relative to this.

### 6. Code Generation
```c
generator_t g = new_generator(output_path, lib_path);
if (target == ZXN) g.target = TARGET_ZXN;
transpile(&g, program);
```
Walks the AST, emits C to the output file. See [[generator/generator-overview]].

### 7. Cleanup
```c
kill_compiler_stack();
```
Frees the entire arena. All strings, AST nodes, and tables are freed in one call.

## Include Resolution Detail

Include resolution happens inside the parser during step 5:

```
parse_include():
  resolve "path/to/file.rkr" relative to project_root
  check circular-include set (error if already included)
  new_lexer(resolved_path) → lex_program() → token_array_t
  splice new tokens into parent token array at current cursor
  continue parsing from splice point
```

Included files are lexed in their entirety before being spliced. The parser sees a seamless flat token stream.

## Error Handling

There is no structured error recovery. Errors at any phase print a message to stderr and call `exit(1)`. The arena is not explicitly freed on error (OS reclaims it).

## Output File

The generator writes to `<output-base>.c`. The file is created/truncated at the start of generation. On host, this file is then passed to `gcc` by `rockc`. On ZXN, it is passed to `zcc`.

## Memory Lifecycle Summary

```
init_compiler_stack()
  │
  ├── lex_program()     -- tokens allocated in arena
  ├── parse_program()   -- AST nodes allocated in arena
  ├── transpile()       -- generator temporaries in arena
  │                        (output written to file, not arena)
  │
kill_compiler_stack()   -- all arena memory freed
```

The output `.c` file is the only persistent output. All intermediate data structures (tokens, AST) are transient within the arena.

See [[lexer/lexer-overview]], [[parser/parser-overview]], [[generator/generator-overview]] for phase detail, and [[overview]] for the architecture diagram.
