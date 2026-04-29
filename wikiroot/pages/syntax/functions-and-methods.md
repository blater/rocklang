---
title: Functions and Methods
category: syntax
tags: [functions, methods, sub, return, method-call, this]
sources: []
updated: 2026-04-27
status: current
---

# Functions and Methods

## Function Definition

```rock
sub name(type1 param1, type2 param2) returns returnType {
  // body
  return value;
}
```

- `sub` introduces a function.
- Parameters require explicit type annotations in **type-first** form (`type name`).
- Return type, when present, is introduced by the `returns` keyword after the closing `)`.
- If `returns` is omitted, the return type defaults to `void`.
- `return expr;` exits the function with a value.

```rock
sub add(int a, int b) returns int {
  return a + b;
}

sub multiply(int a, int b) returns int {
  return a * b;
}

sub greet(string name) {
  print(concat("Hello, ", name));
}
```

## Function Calls

```rock
result := add(3, 4);
greet("World");
```

Arguments are passed positionally. No named parameters.

## Method Definition

### Instance method (on a type)
```rock
sub TypeName.methodName(type param) returns returnType {
  // 'this' refers to the receiver
}
```

`this` is an implicit first parameter of type `TypeName`. The method is callable as `instance.methodName(args)`.

```rock
sub string.shout() returns string {
  return concat(this, "!");
}

string s := "hello";
string loud := s.shout();   // "hello!"
```

### Array method (on a typed array)
```rock
sub TypeName[].methodName(type param) returns returnType {
  // 'this' refers to the array
}
```

```rock
sub int[].sum() returns int {
  int total := 0;
  for n in this {
    total := total + n;
  }
  return total;
}
```

## Method Call Syntax

```rock
receiver.methodName(args)
```

Method calls can be chained:
```rock
s.trim().shout().repeat(3)
```

## Name Mangling

Methods are emitted as free C functions with mangled names:

| Rock | Generated C |
|------|------------|
| `sub Foo.bar(int x)` | `void Foo_bar(Foo this, int x)` |
| `sub Foo[].baz()` | `void Foo_array_baz(Foo_array this)` |

The `this` parameter is the first generated C parameter.

## Scope and Visibility

Functions defined at the top level are visible throughout the file. Included files' functions are visible after the `include` directive. There is no function-level scoping (no closures, no nested functions).

> **TODO:** Nested sub definitions are still unimplemented; `generator.c` currently asserts `sub.path.length == 1`.

## Recursion

Direct recursion is supported. Forward declarations are not needed within a single file (the generator emits forward declarations automatically).

## Example: Record Method

```rock
record Point { int x, int y }

sub Point.translate(int dx, int dy) returns Point {
  return { x := this.x + dx, y := this.y + dy };
}

Point p := { x := 1, y := 2 };
Point q := p.translate(3, 4);
```

See [[syntax/types]] for type syntax, [[syntax/modules-and-records]] for record/module definitions, and [[syntax/arrays]] for array method patterns.
