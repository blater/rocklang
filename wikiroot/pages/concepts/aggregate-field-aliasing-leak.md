---
title: Aggregate Field Aliasing Leak
category: concepts
tags: [memory-model, refcount, aggregate, record, union, leak, uaf, phase-e]
sources: []
updated: 2026-05-01
status: draft
---

# Aggregate Field Aliasing Leak

## Status

Outstanding. Phase E (commit `088e6fd`, 2026-05-01) closed the **scalar-slot** retain/release gap: identifier reassignment, field reassignment, and vardef init all now retain on borrower RHS and release on overwrite for both string and aggregate (record/union/module) handles. The fix did **not** touch how aggregate handles end up *inside* records via record literals or how a record's lifetime relates to the lifetimes of its aggregate-typed fields.

This page describes the residual gap, the four concrete bug shapes it produces, and the design work needed to close it.

## Root Cause

Aggregate field initialisation in record literals is a raw pointer copy with no retain. Two things flow from that:

1. **No retain on field-init from a borrower.** `Outer o := {i := ii, n := 99};` lowers to `o->i = ii;` (a struct-copy through `tmp_o`). `ii` and `o.i` now alias the same Inner block, but the block's refcount is unchanged. Only one of the two pointers carries an rc share; the other is a silent borrower.

2. **No field-walking destructor.** When `__handle_release` frees a record's storage, it does not iterate the record's aggregate or string fields and release them. So if a record is the last live reference to an aggregate field's block, that block leaks. Conversely, if a record's field outlives its source, the source's release frees the block while the record's pointer still aliases it.

Together these mean an aggregate field is essentially a raw, untracked pointer once the record literal is constructed. Whether the program is correct depends on accidental ordering of releases at scope exit.

## Bug Shapes

### Shape 1 — Source reassigned mid-scope (UAF on field read)

```
Inner ii := mkInner(7);
Outer o := {i := ii, n := 0};
ii := mkInner(99);            // releases the old ii; o.i now dangles
print(o.i.x);                 // UAF
```

The reassignment of `ii` releases its old block (the one `o.i` aliases). Subsequent reads through `o.i` hit freed memory.

### Shape 2 — Returning a record with a borrower field (UAF in caller)

```
sub mkOuter(int n) returns Outer {
  Inner ii := mkInner(7);
  Outer o := {i := ii, n := n};
  return o;                   // ii released by callee scope cleanup → o.i dangles
}

sub main() {
  Outer ret := mkOuter(99);
  print(ret.i.x);             // UAF: ret.i was freed when mkOuter's ii went out of scope
}
```

`__return_handle(o)` retains the Outer block, but the Inner block's refcount is unchanged. Callee scope cleanup releases `ii` to zero, freeing it. The caller's `ret.i` is dangling on entry.

### Shape 3 — Record outlives its source field but cannot release (leak)

If we ever fix Shape 2 by retaining on field-init from a borrower (so `o.i = ii; __handle_retain(ii);`), the Inner block reaches rc=2. Without a field-walking destructor on Outer, releasing Outer leaves the Inner block at rc=1 forever — a leak.

Concretely:
```
sub holdInside() returns Outer {
  Inner ii := mkInner(7);     // ii rc=1
  Outer o := {i := ii, n := 0};  // with retain: ii rc=2
  return o;                   // callee releases ii → ii rc=1; o lives on
}                             // caller eventually releases o → Outer freed, Inner block leaks at rc=1
```

So Shape 1+2 and Shape 3 must be fixed together: retain on field-init **and** field-walking destructor.

### Shape 4 — Field overwrite releases an unretained alias (double-free)

```
Inner ii := mkInner(7);       // ii rc=1
Outer o := {i := ii, n := 0}; // o.i = ii; no retain
o.i := mkInner(99);           // current generator: __handle_release(o.i) → ii rc=0, freed
print(ii.x);                  // UAF, or release(ii) at scope cleanup hits already-freed
```

The Phase E field-overwrite path emits `__handle_release(o.i)` before assignment. That release decrements the Inner block's refcount, but no symmetric retain ever ran for `o.i`. The release is "stolen" from `ii`'s share.

Same shape applies to string fields (`record P { string name }; P p := {name := someStr}` — `someStr`'s backing has no retain for `p.name`).

## Why Strings Avoid Most of This Today

Strings have a legacy escape valve: `new_string` deep-copies the source bytes during record literal construction (see `generator.c` around the `Deep-copy string fields to prevent aliasing` comment in `generate_vardef`'s `record_expr` branch). The deep copy gives `p.name` independent storage with its own backing, so source/holder lifetime are decoupled by brute force.

This is exactly what ADR-0003 §H is meant to remove: the legacy `new_string` path is scheduled for deletion once `backing` becomes the canonical lifetime marker. Once it goes, strings will have the same Shape 1–4 problems aggregates have today — the fix needs to land first.

## Required Design Work

1. **Retain on aggregate/string field-init in record literals.** In the `record_expr` arm of `generate_vardef`, after the struct-copy that populates `tmp_o`, emit a retain for each aggregate-typed and string-typed field whose source is a borrower expression. (Producer field expressions — funcalls returning a fresh handle — do not need the retain.)

2. **Generated field-walking destructor per record/union/module type.** A `__release_T` per heap-allocated user type that releases each aggregate and string field, then calls `__handle_release` on the storage itself. Replace the bare `__handle_release` emit at scope cleanup, return cleanup, and field-overwrite with `__release_T`. ADR-0003 §17 Phase E item "Generate `__retain_T` and `__release_T` for every used record, union, and module type" (currently marked done because the *handle* retain/release works) should re-open for the **field-walking** half.

3. **Recursion guard for self-referential records.** A record holding a field of its own type would recurse forever in its destructor. Need either a refcount-aware visit (the freelist marker `ROCK_RC_FREE` already exists) or a structural guard against true self-reference (the negative test `recursive_record_direct.rkr` already rejects direct cycles).

4. **Union variant payload selection.** Unions store one of N payload types. The generated `__release_T` for a union must dispatch on the discriminator and only release the live variant's payload.

5. **Re-evaluate the field-overwrite release.** Once field-init retains, the existing `__handle_release(o.i)` at field overwrite becomes correct (it now matches a real retain). Until then, it's the source of Shape 4 — consider gating on whether field-init retain is in place.

## Tests

Pinning tests live under `test/intentionalFail/`:

- `aggregate_field_alias_source_reassign_test.rkr` — Shape 1 (UAF after source reassignment).
- `aggregate_field_alias_return_test.rkr` — Shape 2 (returning a record with borrower field).
- `aggregate_field_overwrite_double_free_test.rkr` — Shape 4 (field overwrite without prior retain).

These are expected to fail today. As each gets fixed, move it into `test/` and remove the corresponding entry here.

## Related

- [[ADR-0003-memory-model]] — overall two-pool refcount design.
- [[ADR-0003-implementation-plan]] — phase tracking. Phase E item "retain/release on assignment, overwrite, slot write, field write…" needs the field-init half added.
- `test/nested_borrower_test.rkr` — passes today because the borrower retain at `Inner alias := o.i` keeps the Inner block live until `ii`'s release; doesn't exercise Shapes 1–4.
