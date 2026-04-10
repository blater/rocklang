---
title: Records, Product Types, Enums, and Modules
category: syntax
tags: [record, pro, enum, module, composite-types, tdef]
sources: []
updated: 2026-04-09
status: current
---

# Records, Product Types, Enums, and Modules

## record

A `record` is a named composite value type (C struct).

```rock
record Point { x: int, y: int }
record Person { name: string, age: int }
```

### Instantiation

```rock
dim p: Point := record { x := 3, y := 4 };
```

All fields must be initialised explicitly. There is no default initialisation for records.

### Field Access

```rock
let px: int := p.x;
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
let empty: Optional := None;
let full: Optional := Some(42);
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
dim c: Colour := Red;
if c = Blue then print("blue");
```

**Known limitation:** `enum` generates a syntax that is incompatible with SDCC on the ZXN target (`enum_test.rkr` fails on ZXN). All enum tests pass on the host target.

## module

A `module` declares a named singleton struct type. Instance fields are declared as `dim` variables immediately following the `module Name;` declaration within the same file.

```rock
module GameState;

dim score: int;
dim lives: byte;
dim playerName: string;
```

The generator synthesises a `GameState_new()` constructor that zero-initialises all fields. Module instance variables declared in code are initialised with a `GameState_new()` call, deferred to `main()` to ensure all globals are ready.

```rock
dim state: GameState;
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
