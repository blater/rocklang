---
title: "ADR-0001: Function overloading is arity-only"
category: decisions
tags: [adr, generator, overloading, name-resolution]
sources: [src/generator.c, src/name_table.c, test/overload_test.rkr]
updated: 2026-04-13
status: current
---

# ADR-0001: Function overloading is arity-only

**Status:** Accepted (2026-04-13) — Phase 1 shipped.

## Context

Rock's RTL is growing (keyboard, border, print_at so far). The natural API split between a one-arg `print(text)` and a three-arg `print(x, y, text)` ran into Rock's name-only dispatch: call-site resolution used `get_ref(name)` which returned the single name-table entry and emitted the lexeme verbatim as the C symbol. Two functions with the same Rock name could not coexist.

Two strategies were on the table:

1. **Arity-only** — distinguish overloads purely by argument count. Works for every concrete collision in flight. Trivial to implement: detect overload by counting top-level fundef AST nodes with the same name, mangle to `name__N` when count > 1. No typechecker pass required.
2. **Arity + parameter types** — distinguish overloads by the full signature (arity and parameter types). General, familiar from mainstream languages. But Rock has no real typechecker pass today; `infer_expr_type` in `src/generator.c` is a best-effort walker run at codegen time. Full type-based dispatch would require either trusting that walker for correctness (fragile) or wiring a proper resolver pass (a much larger change).

## Decision

Ship arity-only overloading first. Upgrade to arity+types only when a concrete use case demands two overloads at the same arity.

The mangling rule is **`name__N`** (where `N` is the arity) applied only when a Rock name has ≥ 2 top-level user fundefs. Single-definition names keep their bare C symbol so existing projects diff as unchanged.

## Consequences

### Positives

- Unblocks `print` / `print_at` unification (Phase 2 follow-up).
- Every overload collision likely to appear in the next several RTL components is covered.
- No typechecker pass, no new AST nodes, no parser changes — the entire feature is a ~30-line addition to `src/generator.c` plus one program-AST walk helper.
- Backward-compatible: single-definition names still emit bare, so no generated-C diff churn and no symbol-map drift for existing Rock projects.
- Mangling is a pure generator concern; the lexer, parser, and name table are unchanged.

### Negatives / known limitations

- Two overloads **at the same arity** with different parameter types (e.g. `draw(sprite)` vs `draw(point)`) are **not supported**. This is the boundary where Phase 3 (type-based) would be needed.
- `infer_expr_type` returns the last-seen name-table entry for a given Rock name and does not understand overloads. If two overloads of a name disagree on return type, expressions using the call may infer the wrong type. Most real overloads share a return type; we will revisit if this bites.
- User functions still cannot override builtins — the hardcoded `svcmp` fast-paths at the top of `generate_funcall` (append/get/set/pop/insert/substring/concat/toString/printf) are matched before the name table is consulted, and those names are effectively reserved. Unchanged by this ADR.

## Upgrade path to full type-based dispatch (Phase 3, deferred)

When we need overloads at the same arity:

1. Introduce a real semantic pass (either a light typechecker or an extension to `infer_expr_type` that handles calls natively).
2. Extend the mangling scheme to include type tags alongside arity (e.g. `name__2_int_string`).
3. Extend `program_user_fundef_count` → `resolve_user_fundef(name, arg_types)`.
4. All three emission sites (forward decl, definition, call) already route through `emit_fun_name` — they become the single update point.

No Phase 3 work is scheduled.

## See Also

- [[generator-overview]] — arity-based mangling rule documented under "Function Overloading"
- [[rtl-print-at]] — the component whose planned `print(x,y,text)` overload motivated this work
