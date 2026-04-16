---
title: Control Flow
category: syntax
tags: [control-flow, if, while, for, match, loop, iter]
sources: []
updated: 2026-04-16
status: current
---

# Control Flow

## if / then / else

```rock
if condition then
  statement
else
  statement
```

Both branches can be compound blocks:
```rock
if x > 0 then {
  print("positive");
} else {
  print("non-positive");
}
```

`else` is optional. No `elseif` keyword — chain with nested `if`:
```rock
if x > 0 then
  print("positive")
else if x < 0 then
  print("negative")
else
  print("zero");
```

## while

```rock
while condition statement
while condition do statement
```

`do` is optional syntactic sugar.

```rock
int i := 0;
while i < 10 do {
  print(to_string(i));
  i := i + 1;
}
```

## for (counter loop)

```rock
for i := start to end statement
```

Inclusive range: iterates `i` from `start` to `end` (both inclusive). `i` is created in a new scope — not pre-declared.

```rock
for i := 0 to 9 {
  print(to_string(i));
}
```

Generates C: `for(int i = 0; i <= 9; i++) { ... }`

## for / in (iterator loop)

```rock
for x in arr statement
iter x := arr statement    // alternate syntax, same behaviour
```

Iterates over all elements of array `arr`. `x` takes the value of each element in turn.

```rock
int[] nums := [];
append(nums, 10);
append(nums, 20);
append(nums, 30);

for n in nums {
  print(to_string(n));
}
```

## match

```rock
match expr {
  -> value1 statement,
  -> value2 statement
}
```

The parser accepts `match`, but code generation is not implemented: `generate_statement()` asserts when it sees a `match` node. Keep `match` examples as parser-facing syntax until generator semantics are added.

```rock
match colour {
  -> Red   print("red"),
  -> Green print("green"),
  -> Blue  print("blue")
}
```

`match` is typically used with `enum` values.

## Operators

### Arithmetic
```rock
a + b    a - b    a * b    a / b    a % b
```

### Comparison
```rock
a = b    a != b    a < b    a <= b    a > b    a >= b
```

Note: `=` is equality test (not assignment). Assignment uses `:=`.

### Logical
```rock
a && b    a || b
```

### Bitwise
```rock
a & b    a | b    a ^ b
```

### Unary
```rock
-a    // numeric negation
```

### Operators Rock does NOT have

Common C operators that are **absent** from Rock — reaching for these in tests or user code produces confusing parse errors:

| Missing | Use instead |
|---------|-------------|
| `==` equality | `=` (see note above) |
| `!` logical not | `not` is also absent — invert the branch or compare explicitly |
| `and` / `or` keywords | `&&` / `\|\|` |
| `?:` ternary | plain `if` statement writing to a temporary |
| `+=`, `-=`, `*=`, `/=` | `x := x + y` etc. |
| `++`, `--` | `x := x + 1` |

See also [[syntax/types]] for the decimal-only numeric literal rule (no `0x` hex).

## Assignment

```rock
name := expr;              // variable assignment
arr[idx] := expr;          // array element assignment
record.field := expr;      // field assignment
```

Assignment is a statement, not an expression. Cannot be nested inside another expression.

## peek / poke

Low-level memory access, primarily useful on ZXN:
```rock
word addr := to_word(23552);
byte val := peek(addr);   // read byte from address
poke(addr, to_byte(42));  // write byte to address
```

See [[targets/zxn-z80]] for ZXN context and [[syntax/syntax-index]] for full built-in function reference.
