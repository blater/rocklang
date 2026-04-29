---
title: Records, Enums, Unions, and Modules
category: syntax
tags: [record, enum, union, module, composite-types, tdef]
sources: []
updated: 2026-04-27
status: current
---

# Records, Enums, Unions, and Modules

## record

A `record` is a named composite value type (C struct).

```rock
record Point { int x, int y }
record Person { string name, int age }
```

Record fields use **type-first** syntax (`type name`), the same shape as variable declarations and sub parameters:

```rock
record Pair {
  int left,
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
sub Point.magnitude() returns int {
  return (this.x * this.x) + (this.y * this.y);
}
```

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

## union

A `union` is a value that holds **exactly one of N named variants**. Each variant can carry an optional payload of its own type, written **type-first** before the variant name. The compiler auto-generates a constructor function for each variant.

```rock
union Optional { int Some, None }
union Shape    { int Circle, int Rectangle }
union Result   { int Ok, string Err }
```

Variants without a payload type get an implicit `void` payload (no constructor argument).

The compiler emits a struct with a `key` field (an enum of all variant names — tells you which variant is active) and a `value` field (a union of all payload types). See [[concepts/generator/generator-overview]] for the lowered C representation.

### Construction

Constructor functions are auto-generated for each variant:

```rock
Optional opt := Some(42);
Optional empty := None();
```

The generator emits `TypeName_VariantName(payload)` C functions. In Rock, you call variants by bare name: `Some(42)` resolves to `Optional_Some(42)` automatically via name-table lookup of `NT_ENUM_VARIANT` entries.

### Generated C

```c
struct Optional {
  enum { Some, None } key;
  union { int Some; } value;
};

Optional Optional_Some(int payload) {
  Optional __inst = allocate_compiler_persistent(sizeof(struct Optional));
  __inst->key = Some;
  __inst->value.Some = payload;
  return __inst;
}

Optional Optional_None(void) {
  Optional __inst = allocate_compiler_persistent(sizeof(struct Optional));
  __inst->key = None;
  return __inst;
}
```

### Key Access and Case

The active variant can be read directly via `.key`:
```rock
if opt.key = Some then print("has value");
```

`case` on a union compares `->key` automatically:
```rock
case opt {
  Some : print("has value");
  None : print("empty");
  default: print("fallback");
}
```

> **TODO:** `case` on unions currently matches the key only. Destructuring the payload (e.g. `Some(x) : use(x)`) is not yet supported. Access the payload via `.value.VariantName` after matching.

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
// state.score, state.lives, state.playerName are initialised
state.score := 100;
```

Modules can have methods:
```rock
sub GameState.reset() {
  this.score := 0;
  this.lives := 3;
}
```

See [[syntax/functions-and-methods]] for method syntax, [[parser-overview]] for how the parser handles `include` + `module Name;`, and [[generator-overview]] for module initialisation deferral.
