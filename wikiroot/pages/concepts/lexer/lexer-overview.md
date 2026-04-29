---
title: Lexer Overview
category: lexer
tags: [lexer, tokenisation, tokens, scanning]
sources: []
updated: 2026-04-25
status: current
---

# Lexer Overview

The **lexer** converts a Rock source file into a flat array of **tokens**. Implemented in `src/lexer.c`, `src/token.c`, `src/token.h`.

## Data Structures

### token_t
```c
typedef struct token_t {
  token_type_t  type;
  string_view   lexeme;    // Zero-copy pointer+length into source buffer
  int           col, line;
  char         *filename;
  char         *embed_body;   // Non-NULL for TOK_EMBED tokens
  char         *embed_lang;   // "c" or "asm"
} token_t;
```

### lexer_t
```c
typedef struct lexer_t {
  int   col, line, cursor, length;
  char *filename, *data;   // data = entire source file contents
} lexer_t;
```

### string_view
A zero-copy substring: `{ char *start; size_t length; }`. The lexer never copies lexeme text — it stores a pointer into the source buffer. This avoids allocations for every token.

## Key Functions

| Function | Purpose |
|----------|---------|
| `step_lexer(lexer_t*)` | Scan and return one token from current cursor position |
| `lex_program(lexer_t*)` | Scan entire file; return `token_array_t` with EOF sentinel |
| `length_of_delimiter(cursor)` | Compute length of multi-char operator at position |
| `type_of_lexeme(sv)` | Map lexeme string to `token_type_t` (keyword or operator) |
| `get_precedence(type)` | Return operator precedence (1–12, lower = tighter binding) |

## Scanning Approach

The lexer is a single-pass, character-by-character state machine with no backtracking. `step_lexer()` advances the cursor to the next non-whitespace, non-comment character, then determines the token type by inspecting the current character:

1. **Whitespace and comments** are skipped inline before token construction.
   - `//` single-line: advance to next newline.
   - `/* */` multi-line: scan for closing `*/`.
2. **Delimiters and operators** are detected with `length_of_delimiter()` which handles multi-character operators (`->`, `=>`, `||`, `&&`, `<=`, `>=`, `!=`, `:=`, `::`).
3. **Numeric literals** (`TOK_NUM_LIT`): scan digits until non-digit.
4. **String literals** (`TOK_STR_LIT`): scan `"…"` with backslash escape handling (`\n`, `\t`, `\r`, `\\`, `\"`).
5. **Char literals** (`TOK_CHR_LIT`): scan `'…'` with escape handling.
6. **Identifiers and keywords** (`TOK_IDENTIFIER` or keyword token): scan alphanumeric/underscore run, then call `type_of_lexeme()` to check against keyword table.
7. **Embed blocks** (`TOK_EMBED`): triggered by `@embed`. Scans until `@end <lang>` and stores body verbatim in `embed_body`; `embed_lang` is set to `"c"` or `"asm"`.

## Token Types

### Keywords
`TOK_IF`, `TOK_THEN`, `TOK_ELSE`, `TOK_REC`, `TOK_MATCH` (keyword `case`), `TOK_DEFAULT`, `TOK_RETURN`, `TOK_LOOP`, `TOK_WHILE`, `TOK_FOR`, `TOK_INCLUDE`, `TOK_ENUM`, `TOK_SUB`, `TOK_MODULE`, `TOK_EMBED`, `TOK_END`

### Type keywords
`TOK_INT`, `TOK_BYTE`, `TOK_WORD`, `TOK_DWORD`, `TOK_STRING`, `TOK_CHAR`, `TOK_BOOLEAN`, `TOK_VOID`

### Literals
`TOK_NUM_LIT`, `TOK_STR_LIT`, `TOK_CHR_LIT`, `TOK_NULL`

### Identifiers
`TOK_IDENTIFIER` — any alphanumeric/underscore sequence not matching a keyword.

### Operators (binary)
`TOK_PLUS`, `TOK_MINUS`, `TOK_STAR`, `TOK_SLASH`, `TOK_PERCENT`, `TOK_OR`, `TOK_AND`, `TOK_BIT_OR`, `TOK_BIT_XOR`, `TOK_BIT_AND`, `TOK_LT`, `TOK_LTE`, `TOK_GT`, `TOK_GTE`, `TOK_EQ`, `TOK_NEQ`

### Assignment / binding
`TOK_ASSIGN` (`:=`), `TOK_ARROW` (`->`), `TOK_FAT_ARROW` (`=>`), `TOK_DOUBLE_COLON` (`::`), `TOK_COLON` (`:`)

### Delimiters
`TOK_LPAREN`, `TOK_RPAREN`, `TOK_LBRACE`, `TOK_RBRACE`, `TOK_LBRACKET`, `TOK_RBRACKET`, `TOK_SEMICOLON`, `TOK_COMMA`, `TOK_DOT`

### Special
`TOK_EOF` — sentinel placed at the end of every token array.

## Operator Precedence Table

`get_precedence()` returns an integer; **lower value = tighter binding** (evaluated first).

| Precedence | Operators |
|-----------|-----------|
| 1 | Unary `-` (handled in parser, not here) |
| 2 | `*`, `/`, `%` |
| 3 | `+`, `-` |
| 4 | `<`, `<=`, `>`, `>=` |
| 5 | `=` (equals), `!=` |
| 6 | `&` (bitwise AND) |
| 7 | `^` (bitwise XOR) |
| 8 | `\|` (bitwise OR) |
| 9 | `&&` |
| 10 | `\|\|` |

## Escape Sequences

Supported inside string and char literals:

| Escape | Meaning |
|--------|---------|
| `\\` | Backslash |
| `\"` | Double quote |
| `\'` | Single quote |
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |

## Design Notes

- **Zero-copy lexemes**: string views hold pointers into the raw source buffer. The source buffer must remain live for the lifetime of the token array.
- **Flat array output**: `lex_program()` returns a single contiguous `token_array_t`. The parser accesses tokens by index, not iterator.
- **Single lookahead**: The lexer produces one token per `step_lexer()` call; the parser is responsible for all lookahead (it indexes the array directly).
- **No error recovery**: Invalid characters cause the lexer to stop with an error message.

See [[parser-overview]] for how the token array is consumed, and [[glossary]] for term definitions.
