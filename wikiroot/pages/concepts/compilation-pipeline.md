---
title: Compilation Pipeline
category: concepts
tags: [pipeline, main, argv, lib-path, include-resolution, arena]
sources: []
updated: 2026-04-10
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
input = get_abs_path(input, NULL);
```
`main.c` normalises the input path before lexing. The generated C uses relative runtime header names; the surrounding `rock` driver supplies `-I "$ROCK_ROOT/src/lib"` when it invokes `gcc` or `zcc`.

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
parser_t p = new_parser(prog);
p.source = l.data;
p.source_length = l.length;
parse_program(&p);
```
Consumes tokens, resolves includes (recursively lexing + splicing), builds the AST.

Included file paths resolve relative to the including file's directory.

### 6. Code Generation
```c
char *cout = allocate_compiler_persistent(strlen(output) + 3);
sprintf(cout, "%s.c", output);
generator_t g = new_generator(cout);
if (target == ZXN) g.target = TARGET_ZXN;
transpile(&g, p.prog);
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
parse_program():
  resolve "path/to/file.rkr" relative to the including file's directory
  check circular-include set (error if already included)
  new_lexer(resolved_path) → lex_program() → token_array_t
  require included stream to begin with `module Name;`
  splice new tokens into the parent token array at the current cursor
  continue parsing from the splice point
```

Included files are lexed in their entirety before being spliced. The parser sees a seamless flat token stream.

## Error Handling

There is no structured error recovery. Errors at any phase print a message to stderr and call `exit(1)`. The arena is not explicitly freed on error (OS reclaims it).

## Output File

The generator writes to `<output-base>.c`. The file is created/truncated at the start of generation. The surrounding `rock` driver then passes that file to `gcc` on host builds or `zcc` on ZXN builds.

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
