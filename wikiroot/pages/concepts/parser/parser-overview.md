---
title: Parser Overview
category: parser
tags: [parser, ast, grammar, precedence-climbing, include]
sources: []
updated: 2026-04-25
status: current
---

# Parser Overview

The **parser** consumes the flat `token_array_t` from the lexer and produces an **AST** rooted at a `program` node. Implemented in `src/parser.c`, `src/ast.c`, `src/ast.h`.

## Data Structures

### parser_t
```c
typedef struct parser_t {
  token_array_t tokens;
  ast_t         prog;
  int           cursor;
  int           scope_depth;   // 0 = top-level; >0 = inside function/loop
  char         *source;        // Raw source data (used for embed block processing)
  int           source_length; // Length of raw source data
} parser_t;
```

### AST Node Tags (node_tag_t)
```
program       — root; contains array of top-level nodes
fundef        — function/method definition
funcall       — function call
vardef        — variable declaration (type-first style)
assign        — assignment statement
ret           — return statement
op            — binary operator expression
unary_op      — unary minus
literal       — number/string/char/null literal
identifier    — name reference
ifstmt        — if/then/else
while_loop    — while loop
loop          — for i := N to M (counter loop)
iter_loop     — for x in arr (iterator loop)
match         — match expression
matchcase     — single case arm
compound      — { statements… } block
tdef          — type definition (record, union, module)
enum_tdef     — plain enum definition (named integer constants)
cons          — product type constructor application
record_expr   — record literal { field := val, … }
type          — type reference node
arr_index     — array index expression arr[i]
embed         — @embed … @end block
method_call   — receiver.method(args)
sub           — field access / postfix member access chain
```

## Key Functions

| Function | Responsibility |
|----------|---------------|
| `parse_program(parser_t*)` | Top-level entry; handles `include` splicing and loops calling `parse_statement()` until EOF |
| `parse_statement()` | Dispatch for in-body statements: `if`, `while`, `for`, `return`, `case`, expressions |
| `parse_expression(min_prec)` | Precedence-climbing binary expression parser |
| `parse_primary()` | Atoms: literals, identifiers, parenthesised expressions, unary minus |
| `parse_funcall()` | Parse `name(arg, arg, …)` |
| `parse_new_style_var_def()` | Handle `type name [:= expr];` declarations |
| `parse_type()` | Parse a type annotation: scalar, `Type[]`, `Type[N]` |
| `parse_fundef()` | Parse `sub name(params): rettype { body }` and method variants |

## Grammar Summary

### Top-level items
```
program       ::= item* EOF
item          ::= fundef | tdef | include | embed | module_decl
fundef        ::= 'sub' name ['.' name | '[]' '.' name] '(' params ')' [':' type] compound
tdef          ::= 'record' name '{' fields '}' | 'enum' name '{' cons_fields '}'
                | 'module' name ';' vardef*
include       ::= 'include' STRING_LITERAL ';'
embed         ::= TOK_EMBED   (captured by lexer, body verbatim)
```

### Statements
```
statement     ::= vardef | assign | ifstmt | while_loop | loop | iter_loop
                | match | return | funcall ';' | method_call ';' | embed
vardef        ::= type name [':=' expr] ';'
assign        ::= name ':=' expr ';'
                | arr_index ':=' expr ';'
                | receiver '.' field ':=' expr ';'
ifstmt        ::= 'if' expr 'then' statement ['else' statement]
while_loop    ::= 'while' expr ['do'] statement
loop          ::= 'for' name ':=' expr 'to' expr statement
iter_loop     ::= 'for' name 'in' expr statement
              |   'iter' name ':=' expr statement
match         ::= 'case' expr '{' (expr ':' statement | 'default' ':' statement)* '}'
return        ::= 'return' expr ';'
compound      ::= '{' statement* '}'
```

### Expressions
```
expr          ::= unary | binary
unary         ::= '-' primary
binary        ::= primary (op primary)* -- precedence-climbing
atom          ::= literal | identifier | '(' expr ')' | funcall
primary       ::= atom (('.' field_or_method) | ('::' field) | ('[' expr ']'))*
```

## Precedence Climbing

`parse_expression(min_prec)` uses standard precedence-climbing:
1. Parse a `primary`.
2. While the next token is a binary operator with precedence ≥ `min_prec`, consume it and recursively parse the right-hand side with `min_prec + 1` (left-associative).
3. Build an `op` node.

Operator precedences are provided by `get_precedence()` in the lexer. See [[lexer-overview]] for the table.

## Variable Declarations

Rock uses C-like type-first declarations:

```rock
int x := 10;
string s;            -- defaults to ""
int[] arr;           -- defaults to empty array
```

Legacy `let` / `dim` declarations are no longer recognised by the parser.

Default initialisation values when no expression is given:
- `int`, `byte`, `word`, `dword` → `0`
- `string` → `""`
- `char` → `'\0'`
- `boolean` → `false`
- `Type[]` → empty dynamic array

## Include Resolution

```
parse_program():
  1. Resolve the included path relative to the including file's directory
  2. Check circular include set — error if already included
  3. Lex the included file → new token_array_t
  4. Splice: insert new tokens at the current cursor position in the parent token array
  5. Continue parsing (the parser doesn't know the splice happened)
```

Included files **must** begin with `module Name;`. This acts as a namespace declaration and is consumed by the parser before the included file's remaining tokens are processed.

## Method Definition Syntax

```rock
sub Type.method(param: type): rettype { body }     -- instance method
sub Type[].method(param: type): rettype { body }   -- array method
```

Both forms create a `fundef` node with `is_method = 1` (or `is_array_method = 1`) and an implicit first parameter `this: Type` (or `this: Type[]`).

Generated C name: `TypeName_methodName` or `TypeName_array_methodName`.

## Postfix Chains

Field access, method calls, and indexing now share one postfix parser path. That means the parser accepts chains such as:

```rock
make_wrapper().Names
h.Data.Items[1]
get(make_wrapper().Names, 0)
```

## Known Limitations / TODOs

- Nested subs are still unsupported, but the current implementation guard is in `generator.c`, not `parser.c`.
- No semantic analysis. Ill-typed programs produce C compiler errors, not Rock errors.
- No error recovery. The first parse error halts compilation.

See [[lexer-overview]] for token types, [[generator-overview]] for how the AST is consumed, and [[glossary]] for term definitions.
