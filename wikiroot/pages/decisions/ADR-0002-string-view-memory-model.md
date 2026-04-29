---
title: "ADR-0002: Strings are immutable views with region-owned storage"
category: decisions
tags: [adr, strings, memory, arrays, runtime, zxn]
sources:
  - src/lib/typedefs.h
  - src/lib/alloc.c
  - src/lib/fundefs.c
  - src/lib/fundefs_internal.c
  - src/generator.c
  - src/generator.h
updated: 2026-04-28
status: superseded
---

# ADR-0002: Strings are immutable views with region-owned storage

**Status:** Superseded (2026-04-28) by [[decisions/ADR-0003-memory-model]].

ADR-0003 covers the broader memory model (named pools, block-scoped regions, refcounted handles, escape-driven implicit promotion) of which the string design is one facet. The two key changes from this draft to the accepted ADR-0003 design:

1. **Strings are descriptors with capacity, not strictly immutable views.** ADR-0003 reinstates `setCharAt` under a `capacity > 0` runtime check. Mutation through aliases is permitted and observable, matching array semantics.
2. **Region machinery is a target-managed, fixed-bounds pool architecture** rather than a meet-in-the-middle bump-and-freelist heap. Inline asm, sprite buffers, and raw data sit in a contractually inviolate user-reserved range.

The remainder of this document is preserved for historical reference.

---

**Original status:** Draft design plan (2026-04-28).

## Context

Rock targets machines with less than 40KB of 8-bit RAM. The current string
implementation pays for robustness with hidden allocation:

- `string` is a descriptor around `data`, `length`, and an `owned` flag.
- The generator emits per-variable `__free_string()` cleanup for scalar strings.
- String assignment and record construction sometimes deep-copy to avoid aliases.
- `string[]` operations currently deep-copy on `append`, `get`, `pop`, and `set`.
- `substring` allocates a fresh buffer even when a view would be enough.

That model is internally inconsistent. It treats strings as cheap descriptors in
some places and owning heap values in others. For a small-memory target, the
cost is too hidden: reading from a `string[]` can allocate, and repeated string
operations can consume RAM in source code that does not look allocating.

## Critical Review Of The Earlier Plan

The direction "make strings shallow by default" is correct, but a clean
implementation cannot leave the old ownership model half-present.

Do **not** make `__free_string()` a no-op and call the job done. That would leave
generated code, runtime APIs, and struct fields saying strings have per-variable
ownership when the language no longer does. The implementation must remove the
ownership machinery at the same time it changes the semantics.

Do **not** keep `owned` as a dormant field. A field named `owned` invites future
code to branch on a model that no longer exists. Remove it from `struct string`
once all generated `.owned` writes and `__free_string()` calls are gone.

Do **not** keep `is_string_array` in array cleanup. Under this model,
`string[]` stores descriptors like any other array element. Freeing an array
frees only the array's element buffer; it never walks elements to free string
bytes.

Do **not** continue routing Rock string printing or file writes through C `%s`
unless the string is known to be null-terminated. View strings make
null-termination an implementation detail, not a type invariant.

## Decision

Rock `string` is an immutable view:

```c
typedef struct string {
  const char *data;
  size_t length;
} string;
```

Copying a `string` copies only the descriptor. Backing bytes are owned by a
storage region, not by the descriptor.

The following operations must not allocate backing bytes:

- assignment
- parameter passing
- return
- record field assignment
- `string[]` `append`, `insert`, `set`, `get`, and `pop`
- `length`, `charAt`, `equals`, `print`, and string `case` matching
- `substring`, when it can point into the original string

The following operations allocate backing bytes:

- `concat`
- `toString`
- host `read_file`
- explicit `clone(s)`
- C-string conversion when a null-terminated buffer is required

`setCharAt` is retired from normal string semantics. It depends on mutable
shared backing storage and conflicts with immutable descriptor copying.

## Storage And Lifetime Model

String backing bytes live in one of these storage classes:

| Storage class | Examples | Lifetime |
|---|---|---|
| Static | string literals, compiler-emitted constant text | whole program |
| Program arena | `concat`, `toString`, `clone`, host `read_file` | until `kill_compiler_stack()` |
| Borrowed external | C interop views | caller-defined; not owned by Rock |

The first implementation should keep the existing program arena as the only
Rock-owned string allocation region. That is simple and predictable. More
advanced arenas can be considered later, but they should be explicit user-facing
storage regions, not hidden per-variable ownership.

Deallocation happens at region boundaries. A `string` descriptor is never freed
individually.

## Aggregate Values And Nested Containers

The same model must apply to every Rock structure. The runtime memory graph is
not a recursive ownership tree. A value can contain descriptors and handles that
point at memory in one or more **regions**; the descriptor does not own the
target allocation.

| Rock shape | Stored representation | Ownership rule |
|---|---|---|
| Scalar variable | Immediate value | No backing allocation |
| `string` | `{ data, length }` descriptor | Borrows bytes from a region |
| `T[]` | Array handle to header and element buffer | Array owns its slots only |
| `record` / `union` / `module` | Handle to generated struct | Fields are shallow descriptors or handles |
| `string[]` | Array slots containing string descriptors | No string-specific copy or free |
| `record[]` | Array slots containing record handles | No recursive record copy or free |
| `record { string[] names }` | Record field containing an array handle | Record does not own the array recursively |
| `record[]` of records with `string[]` fields | Outer array -> record handles -> array handles -> string descriptors | Reclaim by region, not by walking the graph |

This removes the need for special cases such as "arrays of strings" or "records
with string fields". The same operation copies the same thing everywhere:
scalars copy values, strings copy descriptors, arrays copy handles, and records
copy handles.

### Region Classes

The compiler/runtime should distinguish the lifetime target for each allocation:

| Region class | Intended use | Reset point |
|---|---|---|
| Static | String literals and compiler-emitted constants | Never |
| Statement scratch | Temporaries for one statement, such as `printf(upper(toString(n)))` | End of statement |
| Function scratch | Non-escaping temporaries inside a function | Function return or block reset |
| Result region | Values returned to the caller | Controlled by caller |
| Aggregate/storage region | Arrays, records, globals, module state, and data intentionally retained | Explicit clear/reset or program end |
| Program arena | Legacy fallback for the first migration phase | Program end |

An allocating operation must allocate into a known current region. For example,
`toString`, `concat`, `clone`, record construction, union construction, and
array creation all need either an implicit compiler-selected region or an
explicit region argument in the generated C.

### Promotion

When a value produced in a short-lived region is stored in a longer-lived
destination, it must be promoted or rejected. Examples:

```rock
printf(upper(toString(n)))        // statement scratch is enough
append(names, upper(toString(n))) // must promote or clone into names' region
globalName := upper(name)         // must promote or clone into global region
return { name := upper(s) }       // allocate constructed result in caller region
```

For a small-memory target, explicit source-level allocation is preferable to
silent promotion where the cost is non-obvious. The likely Rock surface is:

```rock
append(names, clone(upper(toString(n))))
```

The generated implementation still needs type-directed `clone_to_region` or
`promote_to_region` helpers for cases where the language defines promotion,
such as returning a newly constructed aggregate.

### Where GC-Like Machinery Would Otherwise Be Needed

Without reference counting or garbage collection, the runtime cannot safely
free the old reachable graph when overwriting a long-lived slot if aliases may
exist:

```rock
team.members[0] := upper(name)
team := make_team()
records[3].names := []
```

The deterministic rule is: overwrites replace descriptors/handles only. Old
backing memory remains in its region until that region is reset. Programs that
need reuse must structure storage around explicit regions, fixed-capacity
arrays, or clear/reset operations.

## Required Runtime Changes

### `src/lib/typedefs.h`

Remove `owned` from `struct string`.

Prefer `const char *data` if the compiler/toolchain allows it cleanly. If SDCC
forces `char *`, keep the field mutable in C but preserve immutable Rock
semantics in the API.

### `src/lib/fundefs.c` and `src/lib/fundefs.h`

Remove `__free_string()` as a normal runtime primitive after generated callers
are gone. Do not leave it as a semantic no-op.

Rename `new_string()` internally to a clearly copying helper, such as
`__string_clone()`. Expose the Rock-level operation as `clone(s)`.

Change `substring` to return a view:

```c
out->data = s.data + c_start;
out->length = len;
```

Keep `concat`, `toString`, and host `read_file` as allocating operations. Each
should allocate once and return a descriptor into program-arena storage.

Replace `%s`-based output for Rock strings with length-aware output:

- `print(s)` already loops by length and is compatible with views.
- `write_string_to_file(s, path)` should use `fwrite(s.data, 1, s.length, f)`.
- generated `printf("%s", string_to_cstr(expr))` must not pass arbitrary views
  directly to `%s`.

Split C-string interop into explicit helpers:

- `string_data(s)` or equivalent for low-level pointer access where length is
  also used.
- `string_to_cstr_clone(s)` for APIs that require a null-terminated buffer.

Host file path parameters require a temporary null-terminated C string unless
the argument is known to already be terminated. Keep that conversion local to
host-only file APIs.

### `src/lib/fundefs_internal.c` and `.h`

Make all `string[]` typed wrappers shallow:

```c
void string_push_array(__internal_dynamic_array_t arr, string elem) {
  __internal_push_array(arr, &elem);
}

void string_get_elem(string *out, __internal_dynamic_array_t arr, size_t index) {
  string *src = __internal_get_elem(arr, index);
  *out = *src;
}

void string_set_elem(__internal_dynamic_array_t arr, size_t index, string elem) {
  __internal_set_elem(arr, index, &elem);
}

void string_insert(__internal_dynamic_array_t arr, size_t index, string elem) {
  __internal_insert(arr, index, &elem);
}
```

Remove `is_string_array` from `__internal_free_array()`. Array cleanup frees the
array struct and its element buffer only.

Consider changing `__internal_pop_array()` to an out-parameter form in the same
change or a follow-up:

```c
void __internal_pop_array_into(__internal_dynamic_array_t arr, void *out);
```

The current `pop` helper allocates a copy of the popped element. That is smaller
than the current string byte-copy problem, but it is still an avoidable hidden
allocation.

Add region-aware allocation for arrays. An array header should know the region
used for its element buffer, or the generated wrapper must pass that region into
growth operations. Dynamic-array growth in a pure bump arena leaks the old
buffer until region reset, so fixed-size arrays or explicit capacities should
remain the preferred low-memory form.

## Required Generator Changes

### Remove string ownership tracking

Remove `TRACK_STRING`, `track_string_var()`, `track_string_tmp()`, and all
generated `__free_string()` cleanup.

`emit_scope_cleanup()` should no longer have a string case. `emit_return_cleanup()`
should disappear or be reduced to whatever non-string cleanup remains. Returning
a string descriptor must be no different from returning an `int` descriptor-sized
value.

### Remove string deep-copy special cases

Replace these codegen paths with plain descriptor assignment:

- string-to-string variable initialisation currently calling `new_string()`
- string reassignment currently freeing the old value
- string field copies during record construction
- temporary ownership transfer via `.owned` nullification

Generated string literals and string temporaries can remain as descriptor
variables, but they must not imply ownership of their backing bytes.

### Add region selection and promotion

The generator must choose an allocation region for every allocating expression:

- statement scratch for non-escaping expression temporaries;
- function scratch for locals proven not to escape;
- caller result region for returned strings, arrays, records, and unions;
- aggregate/storage region for values stored into arrays, records, globals, and
  module fields;
- program arena only as an explicit fallback during migration.

Function signatures in generated C may need hidden region parameters for result
allocation. Constructors for records/unions/modules and array creation helpers
must allocate in the selected region instead of always using
`allocate_compiler_persistent()`.

The generator also needs type-directed promotion helpers for nested aggregates.
For a `record[]` containing records with `string[]` fields, promotion must walk
the type shape when the language requires a long-lived copy. In the default
shallow move case, it should copy only descriptors and handles.

### Update array cleanup tracking

Remove `is_string_array` from `tracked_var_t` and `track_array_var()`. The
generator should emit `__internal_free_array(arr)` with one argument.

### Update `printf` emission

The current generated `printf("%s", expr.data)` is only valid for
null-terminated strings. With substring views, this becomes wrong.

Preferred options:

1. For Rock string expressions, emit `print(expr)` instead of C `%s`.
2. If C `printf` formatting is needed later, add a length-aware Rock formatting
   helper rather than exposing raw `%s` for arbitrary views.

## Language Surface Changes

Add:

```rock
clone(s)
```

`clone` explicitly allocates new backing bytes and returns an independent string
view over them.

Retire:

```rock
setCharAt(s, i, c)
new_string(s)
string_to_cstr(s)
```

`new_string` is not a clear Rock-facing name for byte-copy semantics.
`string_to_cstr` is not safe for arbitrary views. If low-level C interop keeps
it, make it an internal helper or rename it to show allocation/null-termination
requirements.

Keep:

```rock
charAt(s, i)
length(s)
equals(a, b)
concat(a, b)
substring(s, start[, end])
toString(x)
```

## Test Plan

Add tests that lock down allocation and aliasing semantics:

- `string[]` `append`, `insert`, `set`, `get`, and `pop` preserve values without
  byte-copying.
- `get` from `string[]` in a loop does not allocate backing bytes.
- `substring` equality and printing work for non-null-terminated views.
- `write_string_to_file` writes exactly `length` bytes.
- `clone` produces independent backing bytes.
- nested aggregates preserve shallow descriptor semantics: records containing
  strings, records containing arrays, arrays of records, and arrays of records
  with string-array fields.
- statement scratch resets prevent growth in loops such as
  `printf(upper(toString(n)))`.
- storing temporary strings into arrays/globals/record fields requires explicit
  clone/promotion or is rejected with a clear diagnostic.
- `setCharAt` is rejected or no longer registered as a builtin.
- generated C contains no `.owned`, `__free_string`, or string-specific array
  cleanup paths.

For implementation verification, use structural checks as well as behaviour:

```sh
rg "owned|__free_string|TRACK_STRING|is_string_array" src
```

After the migration, that command should return no live source matches except
intentional historical documentation.

## Implementation Phases

### Phase 1: Remove ownership machinery

- Remove `owned` from `string`.
- Remove generator string cleanup and return cleanup.
- Remove string deep-copy-on-assignment and record field copying.
- Remove `__free_string()` callers, then remove the function and declaration.

This phase should be committed independently because it changes the ownership
contract everywhere.

### Phase 2: Make array string operations shallow

- Update `string_push_array`, `string_get_elem`, `string_pop_array`,
  `string_set_elem`, and `string_insert`.
- Remove `is_string_array` from array cleanup, tracking, and generated calls.
- Add regression tests for string array reads and updates.

### Phase 3: Make substring a view

- Change substring runtime helpers to descriptor slicing.
- Fix output and interop paths that assumed null termination.
- Add non-null-terminated substring tests.

### Phase 4: Clean language surface

- Add `clone(s)` as the explicit copy operation.
- Remove or de-register `setCharAt`, `new_string`, and unsafe `string_to_cstr`
  from the Rock-facing builtin table.
- Update syntax docs and wiki pages.

### Phase 5: Reduce hidden element-copy allocation

- Replace `__internal_pop_array()` with an out-parameter helper.
- Update generated wrappers to avoid allocating for scalar and descriptor pops.

This is not required to make string bytes shallow, but it completes the same
small-memory design principle.

### Phase 6: Add region-aware aggregate allocation

- Add runtime region handles and mark/reset support for statement scratch.
- Thread hidden result-region parameters through functions that return
  allocated values.
- Allocate arrays, records, unions, and modules in the selected region rather
  than unconditionally in the program arena.
- Implement type-directed promotion for nested aggregates where promotion is
  part of the language contract.
- Prefer explicit `clone` for user-visible long-lived storage so allocation cost
  remains obvious.

## Consequences

### Positives

- Basic string movement becomes predictable and cheap.
- Array reads no longer allocate.
- The runtime has one ownership story: regions own bytes, descriptors do not.
- The model is orthogonal: no parallel `stringbuf` or string-like API surface.
- `substring` becomes almost free.
- Nested aggregate movement has one rule: copy descriptors and handles unless
  an explicit clone/promotion operation says otherwise.

### Costs

- Programs relying on `setCharAt` must change.
- C interop gets stricter because arbitrary strings are no longer guaranteed to
  be null-terminated at `data[length]`.
- Program-arena allocation still accumulates until exit. That is acceptable for
  the first implementation, but long-running programs will eventually need
  explicit region management.
- Dynamic arrays in bump-style regions need explicit capacity planning or they
  retain old buffers until region reset.
- Automatic reclamation of overwritten long-lived aggregate graphs requires
  reference counting or garbage collection, which this ADR deliberately avoids.

## Non-Goals

- No garbage collector.
- No reference counting.
- No implicit copy-on-write.
- No second string-like type in this phase.
- No lifetime analysis pass in the first implementation.

## Open Questions

- Should `string.data` become `const char *` immediately, or should that wait
  until SDCC compatibility is verified?
- Should `clone` allocate in the program arena only, or should a later explicit
  arena API be designed first?
- Should `printf` remain available only for non-string expressions, with strings
  routed through `print`, or should Rock grow its own formatting helper?
- Should storing a scratch string into a long-lived destination auto-promote, or
  should Rock require explicit `clone` at the source site?
- Should arrays carry their owning region in the runtime header, or should the
  generated code pass the region into every mutating operation?
- Should dynamic arrays require an explicit initial capacity on small targets to
  avoid growth waste in bump regions?

## See Also

- [[concepts/string-representation]]
- [[concepts/array-internals]]
- [[syntax/strings]]
- [[syntax/arrays]]
