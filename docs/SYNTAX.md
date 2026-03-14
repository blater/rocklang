# Rock Language Syntax Reference

---

## Data Types

| Type | Size | Range | Notes |
|------|------|-------|-------|
| `int` | 32-bit | -2,147,483,648 to 2,147,483,647 | Signed integer |
| `char` | 8-bit | 0 to 255 | Single character, can store ASCII/Unicode |
| `string` | Dynamic | N/A | Text string with length field |
| `boolean` | 1 byte | 0 (false) or non-zero (true) | Any non-zero int is true |
| `void` | N/A | N/A | No value; functions only |
| `int[]`, `string[]`, `record[]`, etc. | Dynamic or Fixed | N/A | Array of any type; can be dynamic or fixed-size; syntax: `type[]` |
| `record` | Variable | N/A | Struct with named fields |
| `enum` | 32-bit | Integer values | Named integer constants |

---

## Built-in Functions

### I/O

| Function | Signature | Returns |
|----------|-----------|---------|
| `print` | `print(s: string)` | `void` |
| `putchar` | `putchar(c: char)` | `void` |
| `exit` | `exit(code: int)` | `void` |

### String Operations

| Function | Signature | Returns |
|----------|-----------|---------|
| `concat` | `concat(s: string, x)` | `string` |
| `string_of_int` | `string_of_int(n: int)` | `string` |
| `toString` | `toString(n: int)` | `string` |
| `get_nth_char` | `get_nth_char(s: string, n: int)` | `char` |
| `set_nth_char` | `set_nth_char(s: string, n: int, c: char)` | `void` |
| `get_string_length` | `get_string_length(s: string)` | `int` |
| `str_eq` | `str_eq(s1: string, s2: string)` | `int` |
| `string_to_cstr` | `string_to_cstr(s: string)` | `char*` |
| `cstr_to_string` | `cstr_to_string(cstr: char*)` | `string` |
| `new_string` | `new_string(s: string)` | `string` |

### Array Operations

| Function | Signature | Returns |
|----------|-----------|---------|
| `append` | `append(arr: type[], elem: type)` | `void` |
| `get` | `get(arr: type[], index: int)` | `type` |
| `set` | `set(arr: type[], index: int, value: type)` | `void` |
| `pop` | `pop(arr: type[])` | `type` |
| `length` | `length(arr: type[])` | `int` |

### File Operations

| Function | Signature | Returns |
|----------|-----------|---------|
| `read_file` | `read_file(filename: string)` | `string` |
| `write_string_to_file` | `write_string_to_file(s: string, filename: string)` | `void` |
| `get_abs_path` | `get_abs_path(path: string)` | `string` |

### Standard Library (lib/stdlib.rkr)

| Function | Signature | Returns |
|----------|-----------|---------|
| `print_int` | `print_int(n: int)` | `void` |
| `create_string` | `create_string(src: string, offset: int, length: int)` | `string` |
| `cons_str` | `cons_str(src: string, offset: int)` | `string` |

---

## Keywords

### Declaration

| Keyword | Use |
|---------|-----|
| `let` | Declare variable or function |
| `dim` | Declare variable (BASIC-style synonym for `let`) |
| `sub` | Declare function (alias for `let`) |

### Types

| Keyword | Use |
|---------|-----|
| `record` | Declare struct type |
| `enum` | Declare enum type |

### Control Flow

| Keyword | Use |
|---------|-----|
| `if` | Conditional statement |
| `then` | Branch taken if condition is true |
| `else` | Alternative branch |
| `while` | Loop while condition is true |
| `do` | Marks start of loop body (with while) |
| `loop` | Numeric range iteration |
| `iter` | Array element iteration |

### Other

| Keyword | Use |
|---------|-----|
| `return` | Return from function |
| `include` | Include external file |
| `true` | Boolean true (non-zero) |
| `false` | Boolean false (0) |
| `not` | Logical NOT |
| `and` | Logical AND |
| `or` | Logical OR |
| `void` | Return type indicating no value |
| `NULL` | Null placeholder |

---

## Operators

### Arithmetic

| Operator | Precedence | Associativity |
|----------|-----------|---------------|
| `*` | 5 | Left |
| `/` | 5 | Left |
| `%` | 5 | Left |
| `+` | 4 | Left |
| `-` | 4 | Left |

### Comparison

| Operator | Description |
|----------|-------------|
| `=` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less or equal |
| `>=` | Greater or equal |

### Logical

| Operator | Precedence |
|----------|-----------|
| `not` | 2 |
| `and` | 3 |
| `or` | 1 |

### Assignment

| Operator | Precedence |
|----------|-----------|
| `=>` | 0 |
| `:=` | 0 |

### Operator Precedence (highest to lowest)

1. `( )`
2. `not`
3. `and`
4. `or`
5. `*`, `/`, `%`
6. `+`, `-`
7. `<`, `>`, `<=`, `>=`, `=`, `!=`
8. `=>`, `:=`

---

## Variable Declaration

| Form | Syntax |
|------|--------|
| Let with fat arrow | `let name: type => value;` |
| Let with pascal | `let name: type := value;` |
| Dim with fat arrow | `dim name: type => value;` |
| Dim with pascal | `dim name: type := value;` |

**Notes:**
- Type annotation required: `: type`
- Both `=>` and `:=` are equivalent
- `let` and `dim` are equivalent (prefer `let` unless using BASIC style)

---

## Function Declaration

| Form | Syntax |
|------|--------|
| With return type | `let name(param1: type1, param2: type2): return_type => { body }` |
| No parameters | `let name(): return_type => { body }` |
| Returns void | `let name(param: type): void => { body }` |

---

## Records

| Syntax | Description |
|--------|-------------|
| `record Name { field: type }` | Declare record |
| `record Name => { field: type }` | Declare record (alt syntax, => optional) |
| `name::field` | Access field (colon-colon) |
| `name.field` | Access field (dot notation) |
| `name::field => value` | Update field |
| `name.field := value` | Update field |

---

## Enums

| Syntax | Description |
|--------|-------------|
| `enum Name { Variant1, Variant2 }` | Declare enum |
| `enum Name => { Variant1, Variant2 }` | Declare enum (alt syntax, => optional) |

---

## Arrays

| Syntax | Description |
|--------|-------------|
| `type[]` | Array type annotation |
| `let arr: type[] => [];` | Declare empty array |
| `append(arr, elem)` | Add element |
| `get(arr, index)` | Get element at index |
| `set(arr, index, value)` | Set element at index |
| `pop(arr)` | Remove and return last element |
| `length(arr)` | Get array length |
| `iter elem: arr { }` | Iterate over array |

---

## Control Flow Syntax

| Statement | Syntax |
|-----------|--------|
| If-then | `if condition then { }` |
| If-then-else | `if condition then { } else { }` |
| If-then-else-if | `if cond1 then { } else if cond2 then { } else { }` |
| While loop | `while condition do { }` |
| For loop (range) | `loop i: start -> end { }` or `loop i: start -> end => { }` |
| Array iteration | `iter elem: array { }` or `iter elem: array => { }` |

**Notes:**
- `=>` is optional in `loop` and `iter`
- Condition is non-zero for true, zero for false
- No return statement needed for void functions

---

## Comments

| Type | Syntax |
|------|--------|
| Single-line | `// comment` |
| Multi-line | `/* comment */` |

---

## String Literals

| Form | Description |
|------|-------------|
| `"text"` | String literal |
| `"text\n"` | With newline escape |
| `""` | Empty string |

---

## Character Literals

| Form | Description |
|------|-------------|
| `'a'` | Single character |
| `'Z'` | Uppercase character |
| `'\n'` | Newline character |
| `'\0'` | Null character |

---

## Include Syntax

| Syntax | Description |
|--------|-------------|
| `include "path/file.rkr"` | Include external file |
| `include "lib/stdlib.rkr"` | Include standard library |
| `include "lib/file.rkr"` | Include file I/O library |
| `include "lib/term.rkr"` | Include terminal library |

---

## Field Access

| Syntax | Equivalent? | Notes |
|--------|-------------|-------|
| `record::field` | Colon-colon syntax | Recommended |
| `record.field` | Dot notation | Alternative |

Both are identical in behavior.

---

## Assignment Operators

| Operator | Name | Equivalent To |
|----------|------|--------------|
| `=>` | Fat arrow | Assignment |
| `:=` | Pascal gets | Assignment |

Both are identical; use whichever you prefer for consistency.

---

## Special Values

| Value | Type | Meaning |
|-------|------|---------|
| `true` | `int` | Non-zero (typically 1) |
| `false` | `int` | Zero (0) |
| `NULL` | Any | Null/uninitialized placeholder |

---

## Type Casting & Conversion

| Function | Converts | To |
|----------|----------|-----|
| `string_of_int(n)` | `int` | `string` |
| `toString(n)` | `int` | `string` |
| `string_to_cstr(s)` | `string` | `char*` |
| `cstr_to_string(p)` | `char*` | `string` |

---

**Last Updated**: 2026-03-05
