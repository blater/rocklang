---
title: Name Table (Symbol Table)
category: concepts
tags: [name-table, symbol-table, scope, type-inference]
sources: []
updated: 2026-04-09
status: current
---

# Name Table

The **name table** is the compiler's symbol table. It tracks all named entities (variables, functions, types) and their scopes. Implemented in `src/name_table.c`, `src/name_table.h`.

## Data Structure

```c
typedef struct name_table_t {
  nt_kind     *kinds;    // Entry kind for each slot
  string_view *names;    // Name for each slot
  int         *scopes;   // Scope depth for each slot
  ast_array_t  refs;     // Parallel array of AST nodes (the definition)
  int          scope;    // Current scope depth
  int          length;   // Number of active entries
  int          capacity; // Allocated capacity (doubles on overflow)
} name_table_t;
```

### Entry Kinds (nt_kind)
```c
NT_FUN          -- user-defined function
NT_VAR          -- variable (local or global)
NT_BUILTIN_TYPE -- built-in type (int, string, etc.)
NT_USER_TYPE    -- user-defined type (record, module, enum)
```

## Scope Model

The name table uses a **flat array with scope depth tags** — not a tree. Scope depth starts at 0 (top-level / global). Every function entry, loop, or block increments the scope.

```
scope 0: top-level functions and global vars
scope 1: inside a function
scope 2: inside a nested block (loop, if body)
```

### Scope Lifecycle

```c
new_nt_scope(&table);    // scope++
// ... push entries at current depth ...
end_nt_scope(&table);    // scope--; truncate entries with depth > current
```

`end_nt_scope()` simply walks backwards through the entry list and reduces `length` until no entry with a deeper scope remains. Entries are **never individually deleted** — they fall off the end of the active range.

## Operations

### push_nt
```c
void push_nt(name_table_t *t, nt_kind kind, string_view name, ast_t ref);
```
Register a new name at the current scope depth. Doubles capacity if full.

### get_ref
```c
ast_t get_ref(name_table_t *t, string_view name);
```
Linear search from the start of the table. Returns the AST node for the **most recently registered** entry matching `name`. This means later shadowing entries win.

Complexity: O(n) — adequate for small tables typical of Rock programs.

### register_builtin / register_builtin_array
Pre-populates the table with built-in function stubs at scope 0:
```c
register_builtin(&table, "append", "void");
register_builtin(&table, "get",    "int");
register_builtin_array(&table, "get_args", "string");
```

## Role in the Generator

The name table is the generator's primary mechanism for type inference. When the generator encounters an identifier or method call, it calls `get_ref()` to find the declaration node, then examines the node's type field.

**Key use cases:**
- Determining the element type of an array (needed to choose `int_get_elem` vs `string_get_elem`)
- Resolving method call receiver type (needed to mangle the C function name)
- Distinguishing user-defined functions from builtins
- Checking whether a name refers to a module type for deferred initialisation

## Limitations

- **No error on undeclared names.** If `get_ref()` returns NULL, the generator may emit invalid C, producing a C compiler error rather than a Rock-level error.
- **No shadowing warning.** If a name is re-declared in the same scope, the new entry silently shadows the old one.
- **Latest entry wins.** With a flat array and forward-scan search, if the same name appears at multiple scopes, the most recent push wins — this is the correct behaviour for shadowing but is not explicitly documented in the code.

See [[generator-overview]] for how the name table is used during code generation, and [[glossary]] for scope/symbol definitions.
