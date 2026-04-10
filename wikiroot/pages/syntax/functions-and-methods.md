---
title: Functions and Methods
category: syntax
tags: [functions, methods, sub, return, method-call, this]
sources: []
updated: 2026-04-10
status: current
---

# Functions and Methods

## Function Definition

```rock
sub name(param1: type1, param2: type2): returnType {
  -- body
  return value;
}
```

- `sub` introduces a function.
- Parameters require explicit type annotations.
- Return type follows `:` after the closing `)`.
- If return type is omitted, it defaults to `void`.
- `return expr;` exits the function with a value.

```rock
sub add(a: int, b: int): int {
  return a + b;
}

sub greet(name: string): void {
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
sub TypeName.methodName(param: type): returnType {
  -- 'this' refers to the receiver
}
```

`this` is an implicit first parameter of type `TypeName`. The method is callable as `instance.methodName(args)`.

```rock
sub string.shout(): string {
  return concat(this, "!");
}

string s := "hello";
string loud := s.shout();   -- "hello!"
```

### Array method (on a typed array)
```rock
sub TypeName[].methodName(param: type): returnType {
  -- 'this' refers to the array
}
```

```rock
sub int[].sum(): int {
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
| `sub Foo.bar(x: int)` | `void Foo_bar(Foo this, int x)` |
| `sub Foo[].baz()` | `void Foo_array_baz(Foo_array this)` |

The `this` parameter is the first generated C parameter.

## Scope and Visibility

Functions defined at the top level are visible throughout the file. Included files' functions are visible after the `include` directive. There is no function-level scoping (no closures, no nested functions).

> **TODO:** Nested sub definitions are still unimplemented; `generator.c` currently asserts `sub.path.length == 1`.

## Recursion

Direct recursion is supported. Forward declarations are not needed within a single file (the generator emits forward declarations automatically).

## Example: Record Method

```rock
record Point { x: int, y: int }

sub Point.translate(dx: int, dy: int): Point {
  return { x := this.x + dx, y := this.y + dy };
}

Point p := { x := 1, y := 2 };
Point q := p.translate(3, 4);
```

See [[syntax/types]] for type syntax, [[syntax/modules-and-records]] for record/module definitions, and [[syntax/arrays]] for array method patterns.
