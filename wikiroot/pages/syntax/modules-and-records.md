---
title: Records, Product Types, Enums, and Modules
category: syntax
tags: [record, pro, enum, module, composite-types, tdef]
sources: []
updated: 2026-04-10
status: current
---

# Records, Product Types, Enums, and Modules

## record

A `record` is a named composite value type (C struct).

```rock
record Point { x: int, y: int }
record Person { name: string, age: int }
```

Record fields support both the original `name: type` form and the newer type-first form:

```rock
record Pair {
  left: int,
  string right,
}
```

### Instantiation

```rock
Point p := { x := 3, y := 4 };
```

All fields must be initialised explicitly. There is no default initialisation for records.

### Field Access

```rock
int px := p.x;
p.y := 10;
```

### Methods on Records

```rock
sub Point.magnitude(): int {
  return (this.x * this.x) + (this.y * this.y);
}
```

## pro (Product Type)

A `pro` is a tagged-union-like construct. It declares a type with named constructors, each carrying a value.

```rock
pro Optional { Some: int, None }
pro Shape { Circle: int, Rectangle: int }
```

Constructors without a payload use `null`:
```rock
Optional empty := None;
Optional full := Some(42);
```

`match` is typically used to inspect a `pro` value:
```rock
match opt {
  -> Some print(to_string(opt.value)),
  -> None print("empty")
}
```

> **TODO:** The exact match semantics for `pro` types need clarification from the source and tests.

## enum

An `enum` defines a set of named integer constants.

```rock
enum Colour { Red, Green, Blue }
enum Direction { North, South, East, West }
```

Enum values map to C integers (0, 1, 2, …).

```rock
Colour c := Red;
if c = Blue then print("blue");
```

## module

A `module` declares a named singleton struct type. Instance fields are declared as ordinary type-first variables immediately following the `module Name;` declaration within the same file.

```rock
module GameState;

int score;
byte lives;
string playerName;
```

The generator synthesises a `GameState_new()` constructor that zero-initialises all fields. Module instance variables declared in code are initialised with a `GameState_new()` call, deferred to `main()` to ensure all globals are ready.

```rock
GameState state;
-- state.score, state.lives, state.playerName are initialised
state.score := 100;
```

Modules can have methods:
```rock
sub GameState.reset(): void {
  this.score := 0;
  this.lives := 3;
}
```

See [[syntax/functions-and-methods]] for method syntax, [[parser/parser-overview]] for how the parser handles `include` + `module Name;`, and [[generator/generator-overview]] for module initialisation deferral.
