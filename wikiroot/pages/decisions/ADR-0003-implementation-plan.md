---
title: "ADR-0003 implementation plan"
category: decisions
tags: [adr, memory, implementation-plan, checklist, runtime, generator, zxn]
sources:
  - wikiroot/pages/decisions/ADR-0003-memory-model.md
updated: 2026-04-28
status: draft
---

# ADR-0003 implementation plan

This page is the execution layer for [[decisions/ADR-0003-memory-model]].

The ADR is the design contract: what the memory model is and why. This page is the working checklist: how the implementation is sequenced, what must be verified at each stage, and what lessons from implementation must feed back into the plan.

## Operating rules

- **ADR-0003 remains authoritative.** If implementation reveals that the design is wrong, update ADR-0003 and this plan in the same change.
- **This plan is intentionally mutable.** Update it as implementation exposes missing work, better ordering, unexpected coupling, or new regression tests.
- **No silent drift.** Do not carry a workaround, TODO, or changed invariant only in code comments. Record it here, and update the ADR if it changes the model.
- **No partial semantic release.** Phases are build-order milestones, not independently shippable language states.
- **Every phase exits through checks.** A phase is not complete because code exists; it is complete when the listed compile, structural, behavioural, and wiki checks pass.
- **Small commits are allowed; partial semantics are not.** Work may land in a branch as layered commits, but the branch must not be merged until the full ADR-0003 contract passes.

## Checklist discipline

Use this loop during implementation:

1. Before starting a phase, compare the checklist against the current ADR text.
2. During the phase, append any discovered missing task or invariant to this page immediately.
3. When a bug is found, add or update a test checklist item before fixing the bug.
4. At phase exit, mark completed items and add a short implementation note if the code differs from the initial plan.
5. If a lesson changes architecture rather than execution detail, update ADR-0003 first, then adjust this plan.

Checklist markers:

- `[ ]` not started
- `[~]` in progress
- `[x]` complete
- `[!]` blocked or design feedback required

## Whole-change definition of done

- [ ] `make` passes with `-Werror -Wall -Wextra`.
- [ ] Full host test suite passes.
- [ ] Full host test suite passes under `--memory-profile=zxn`.
- [ ] Required ZXN smoke tests build and run.
- [ ] Structural greps from ADR-0003 §16.3 pass with no live source matches.
- [ ] Debug-build leak detector reports zero unexpected longlived refcounts at program exit.
- [ ] No stale generated-code path uses old string `owned` semantics.
- [ ] No stale generated-code path uses hidden caller `__result_region` function ABI.
- [ ] Wiki pages listed in ADR-0003 §See Also are updated or explicitly marked stale.
- [ ] ADR-0003 is promoted from `draft` to `accepted` only after the implementation branch satisfies the above.

## Global invariants

These invariants must hold across every phase. If one is temporarily broken in a branch commit, it must be restored before merge.

- [ ] Strings use `{data, length, capacity, backing}`; no `owned` field remains.
- [ ] Static string literals use a static sentinel backing header with `refcount = 0xFFFF`.
- [ ] Bump-backed strings have `backing = NULL`; retain/release are no-op; promotion or return materialisation copies when needed.
- [ ] Longlived string backing is refcounted and reclaimed at zero.
- [ ] Arrays, records, unions, and modules use the universal block header as the single canonical refcount location.
- [ ] Bump containers still refcount logical aliases and release children exactly once; they do not freelist-free their own physical storage.
- [ ] Longlived containers release children before returning their own storage to freelists.
- [ ] Function parameters are retained on callee entry and released on callee exit.
- [ ] Non-scalar function returns call `__return_T` / `__return_string` before unwind.
- [ ] `__return_T` materialises an owned caller result: static unchanged, longlived retained, bump copied into `longlived`.
- [ ] Store promotion and function return materialisation remain separate mechanisms.
- [ ] Loop-carried composite slots are allocated in `longlived`.
- [ ] Recursive user-defined type graphs are rejected by the typechecker.
- [ ] `reclaim()` coalesces only free adjacent longlived blocks and never relocates live data.

## Phase 0: Baseline and guardrails

- [ ] Capture current `make` and `./run_tests.sh` baseline before memory-model changes.
- [ ] Capture current generated C for representative string, array, record, and return-heavy programs.
- [ ] Add a temporary implementation branch note listing known unrelated failures, if any.
- [ ] Confirm `wikiroot/pages/decisions/ADR-0003-memory-model.md` is the current design target.
- [ ] Add CI or local scripts for the structural greps before broad refactoring starts.

Phase exit checks:

- [ ] Baseline failures, if any, are recorded.
- [ ] No ADR-0003 implementation work has started without a reproducible baseline.

## Phase A: Pool architecture and target budgets

- [ ] Measure ZXN code, rodata, runtime state, stack, and usable pool budget from `.map` output.
- [ ] Replace illustrative ADR pool sizes with measured values before acceptance.
- [x] Implement `__pool_bump_alloc`, `__pool_bump_save`, and `__pool_bump_restore`. — `src/lib/pools.c` (named `rock_bump_*`).
- [x] Implement `__pool_longlived_alloc` and `__pool_longlived_free` with universal `{size, refcount}` headers.
- [x] Implement free-block and static-block sentinels. — `0xFFFE` and `0xFFFF` respectively.
- [x] Implement size-class freelists and miscellaneous-class handling. — Power-of-two classes 8…4096; misc list for non-class sizes.
- [x] Implement `reclaim()` as non-relocating adjacent-free-block coalescing. — `rock_longlived_reclaim`.
- [x] Add named OOM diagnostics for `bump` and `longlived`. — Default handler exits; tests can install a non-exiting capture.
- [ ] Implement `--memory-profile=zxn` for host builds.

Phase exit checks:

- [x] Pool unit tests or focused Rock tests can exhaust each pool with named diagnostics. — `test/pools_test.c` 18/18 passing.
- [x] `reclaim()` tests show adjacent free blocks coalesce and non-adjacent blocks do not.
- [ ] Host `--memory-profile=zxn` uses the same pool sizes as the ZXN manifest.

Implementation note (2026-04-29): function names use `rock_` prefix instead of `__pool_` per existing project naming; semantics are identical. Pool sizes in the generator are placeholders pending the deferred `.map` measurement.

## Phase B: Typechecker acyclicity

- [x] Build the user-type graph after type resolution.
- [x] Walk through array, record, union, and descriptor fields.
- [x] Reject direct recursive types.
- [x] Reject indirect recursive cycles.
- [x] Emit a diagnostic that names the cycle path and recommends flat index-based structures.

Phase exit checks:

- [x] `test/recursive_type_rejected_test.rkr` exists and passes. — Three negative tests under `test/negative/` cover direct, indirect-via-array, and mutual recursion. Runner is `test/negative/run_negative.sh` (`make test-negative`).
- [x] Non-recursive aggregate combinations still typecheck. — All 397 existing tests pass; flat-tree workaround (`Tree { Node[] nodes }, Node { int parent_idx }`) compiles cleanly.

## Phase C: Region emission and cleanup skeleton

- [x] Emit bump save/restore for function bodies. — `generate_compound` covers all braced blocks including function bodies.
- [x] Emit bump save/restore for `if`, `else`, loop, and `case` bodies. — Same path; all braced bodies route through `generate_compound`.
- [x] Emit fresh region handling for each loop iteration. — Loop bodies are compounds, so each iteration reissues `rock_bump_save`/`rock_bump_restore` around the body.
- [ ] Emit statement-region save/restore. — Deferred to Phase G/F. No allocations route to the bump pool yet, so per-statement save/restore would be no-op scaffolding. Will be added when escape analysis decides per-statement scratch lifetimes.
- [x] Add freestanding plain-block statement parsing. — Already supported in `parser.c:501-502` from prior work.
- [ ] Build compile-time cleanup records with typed owned locals. — Existing `tracked_var_t` machinery still drives cleanup; replacement happens in Phase H/J when refcount-based release supersedes `__free_string`.
- [ ] Replace return-only cleanup with `emit_unwind_to(target_scope)`. — Pending; existing `emit_scope_cleanup` still drives cleanup.

Phase exit checks:

- [ ] `test/region_iteration_test.rkr` passes. — Test not yet added; meaningful only once allocations route through the bump pool. Add during Phase G.
- [ ] `test/region_nested_block_test.rkr` passes. — Same: add during Phase G.
- [ ] Generated code for early return restores the correct bump marks in order. — Pending Phase H (early-exit unwinding).

Implementation note (2026-04-29): bump save/restore is emitted in `generate_compound` using `rock_bump_mark __bm` with C scope shadowing; nested blocks shadow the outer `__bm` naturally without needing a counter. Pools runtime is included in every generated program (`#include "pools.h"`, `rock_pools_init`/`rock_pools_deinit` in main); host gets 4 MB pools, ZXN gets 7 KB bump / 3 KB longlived as placeholders.

## Phase D: Descriptor and handle layout

- [~] Replace `struct string.owned` with `capacity` and `backing`. — Partial: capacity and backing fields added; `owned` retained as a transitional compatibility marker. Removal happens in Phase J alongside `__free_string` deletion.
- [ ] Add universal block header access helpers. — Pending; helper to derive `(rock_block_header *)payload - 1` not yet abstracted.
- [x] Ensure arrays do not carry a second refcount in `__internal_dynamic_array_t`. — By inaction; the struct remains `{data, length, capacity, elem_size, max_capacity}` with no refcount field.
- [x] Ensure aggregates do not introduce a separate `__aggregate_header`. — By inaction; no such header was added.
- [~] Update generated allocations to initialise `capacity`, `backing`, and universal-header refcounts. — Partial: every `__rock_make_string`-style site now sets `capacity` and `backing = NULL`; allocation through `rock_longlived_alloc` is wired in Phase H when `concat`/`toString`/`clone` move off `allocate_compiler_persistent`.
- [ ] Emit static string literal backing headers with `refcount = 0xFFFF`. — Pending Phase H.

Phase exit checks:

- [ ] Structural grep shows no live `owned` field usage. — Pending Phase J.
- [ ] Static literal descriptors point at sentinel backing. — Pending Phase H.
- [x] Bump string descriptors use `backing = NULL`. — All current paths set `backing = NULL` because real backing isn't allocated yet.

Implementation note (2026-04-29): Phase D as scoped here is intentionally narrower than the ADR §17 "Phase D" wording — it stops at adding the new fields and initialising them safely. The owned-removal half of ADR §17 Phase D is folded into Phase J, after Phase E (refcount) and Phase H (string semantics) have replaced the legacy cleanup path.

## Phase E: Retain/release runtime and generated walkers

- [x] Implement `__string_retain` and `__string_release`. — `src/lib/fundefs.{c,h}` with three-class discriminant; C-level tests in `test/string_refcount_test.c` cover NULL backing, static sentinel, and longlived inc/dec/free-on-zero/reuse.
- [!] Implement array retain/release using universal headers. — Blocked on Phase D completion: `__internal_dynamic_array_t` does not yet sit inside a universal block header. Needed for the `array_retain/release` pair.
- [x] Implement aggregate retain/release using universal headers. — `__handle_retain` / `__handle_release` in `src/lib/fundefs.{c,h}`; Phase D extension already moved record/union/module allocations onto `rock_longlived_alloc`, so the universal header is in place. C-level tests in `test/string_refcount_test.c` cover NULL, retain inc, release-to-zero free.
- [~] Generate `__retain_T` and `__release_T` for every used record, union, and module type. — All three aggregate kinds use the same header layout, so a single pair (`__handle_retain` / `__handle_release`) serves them all instead of per-type emission. Generator emits `__handle_release` at scope cleanup for every TRACK_RECORD slot. Per-type retain at assignment/overwrite sites still pending (same item as the string equivalent below).
- [!] Generate `__release_array_T` for every used array element type. — Blocked on array header layout.
- [!] Ensure bump-container dec-to-zero releases children but does not freelist-free physical storage. — Blocked on bump-allocated containers having headers (Phase H).
- [!] Ensure longlived-container dec-to-zero releases children, then frees physical storage. — Blocked on the same.
- [~] Emit retain/release on assignment, overwrite, slot write, field write, scope exit, and discarded producer temporaries. — Partial: `__string_release` is paired with every `__free_string` emission (scope-cleanup, return-cleanup, inline string reassignment, inline field reassignment). Producer/borrower-driven retain emission at the new assignment sites is deferred to Phase H, when the legacy `new_string` deep-copy path is replaced and `backing` becomes the canonical lifetime marker.
- [!] Emit callee-side retain/release for every refcounted parameter. — Blocked on Phase F: parameter release on exit must run after `__return_T` materialises the return value, otherwise returning a parameter would release its backing before the caller captures it.

Phase exit checks:

- [!] `test/string_backing_refcount_test.rkr` passes. — Blocked on Phase H (literals with sentinel backing; `concat` allocating via rock_longlived_alloc).
- [!] `test/string_substring_retain_test.rkr` passes. — Blocked on Phase H (substring inheriting source backing).
- [!] `test/record_with_string_release_test.rkr` passes. — Blocked on aggregate headers + per-type walkers.
- [!] `test/array_of_records_with_strings_test.rkr` passes. — Blocked on the same.
- [!] `test/bump_alias_release_test.rkr` passes. — Blocked on bump containers carrying headers.

Implementation note (2026-04-29): Phase E's foundational pieces (E.a runtime helpers, E.b release pairing) landed. The remaining items couple to data layout (Phase H must give literals/concat-results/etc. real `backing`) and to ABI semantics (Phase F must define return materialisation before parameter retain/release is sound). Dependency chain: Phase D-extension → Phase F → Phase H → re-open Phase E to add the new retain emissions and per-type walkers, then verify the five behavioural tests.

## Phase F: Return materialisation

- [x] Implement `__return_string`. — `src/lib/fundefs.c:235`. Three-class discriminant: static pass-through, longlived inc, bump→longlived copy. C-level coverage in `test/string_refcount_test.c`.
- [x] Generate `__return_T` for every returned record, union, and module type. — Single `__return_handle` (universal-header retain) covers all three since Phase D extension routes them through `rock_longlived_alloc`. `generate_return` detects the aggregate case via `ret_type_is_handle_aggregate` and emits `T __retval = __return_handle(<expr>);` followed by cleanup + return.
- [ ] Generate `__return_array_T` for every returned array element type. — Pending; arrays still use `__internal_dynamic_array_t` without a universal header (Phase D array work).
- [x] Ensure static returns are unchanged. — `__return_string` and `__handle_retain` both detect `ROCK_RC_STATIC` and pass through.
- [x] Ensure longlived non-static returns retain once for the caller. — Both `__return_string` and `__return_handle` increment refcount.
- [x] Ensure bump-backed returns allocate-copy into `longlived`. — `__return_string` does this for `backing == NULL`. Aggregates have no bump path yet (always allocated longlived).
- [x] Ensure callee unwind releases the original temp/local/parameter references after `__return_T`. — `emit_return_cleanup` runs after the `__retval` capture and now releases TRACK_STRING and TRACK_RECORD (arrays still skipped, blocked on array-header phase).
- [x] Ensure caller capture treats every non-scalar function result as a producer transfer. — Caller's `T x := f()` is a plain handle/descriptor copy; no extra retain — caller now owns the rc=1 reference produced by the callee's `__return_T`.
- [x] Remove any hidden caller `__result_region` function ABI code path. — `grep -rn __result_region src/` returns no live hits; the symbol never reached implementation.

Phase exit checks:

- [ ] `test/return_borrowed_string_test.rkr` passes.
- [ ] `test/return_longlived_borrower_test.rkr` passes.
- [ ] `test/return_fresh_producer_loop_test.rkr` passes.
- [ ] `test/escape_return_substring_test.rkr` passes.
- [ ] Structural grep confirms no hidden `__result_region` ABI remains.

## Phase G: Store promotion and escape analysis

- [ ] Annotate value-producing expressions with source regions.
- [ ] Annotate store sites with destination regions.
- [ ] Emit store promotion when source lifetime is shorter than destination lifetime.
- [ ] Keep store promotion separate from return materialisation.
- [ ] Implement destination-driven allocation for known local, field, and slot destinations.
- [ ] Implement default statement-scratch allocation for non-escaping temporaries.
- [ ] Implement loop-carried composite slot detection and upgrade such values to `longlived`.
- [ ] Reject uncapacitied arrays that escape to `longlived`.

Phase exit checks:

- [ ] `test/escape_promotion_test.rkr` passes.
- [ ] `test/destination_driven_alloc_test.rkr` passes.
- [ ] `test/loop_carried_longlived_test.rkr` passes.
- [ ] `test/array_fixed_capacity_test.rkr` passes.

## Phase H: String semantics and public API

- [x] Make `substring` return a view with inherited backing. — `__substring_from`/`__substring_range` now return descriptor with `data = source + offset`, `capacity = 0`, `backing = source.backing`, plus `__string_retain` to extend source's lifetime.
- [x] Make `charAt`, `equals`, and `print` length-aware. — All three already used `length` in `fundefs.c`; verified.
- [x] Replace generated C `printf("%s", expr.data)` for Rock strings. — Generator now emits `print(expr)` via the length-aware runtime helper.
- [x] Update file output to use `fwrite(s.data, 1, s.length, f)`. — `write_string_to_file` updated.
- [x] Implement capacity-gated `setCharAt`. — Halts on `capacity == 0` with a clear runtime diagnostic; new negative test `test/negative/setcharat_on_literal.rkr` proves the halt path. `test/negative/run_negative.sh` extended with a runtime-halt mode.
- [!] Replace `new_string` with `clone`. — Blocked on Phase I: `fundefs_internal.c` uses `new_string` for string-array element deep-copy. Renaming requires switching string-array elements to descriptor-copy (ADR §8.7), which is Phase I work.
- [x] Keep one `string` type; do not add `stringbuf` or `char[]` string-like duplicates. — By inaction; only `string` exists.

Phase exit checks:

- [x] String literals get static sentinel backing (Phase H step 1).
- [x] `concat` / `toString` / `clone` / `read_file` / `__get_abs_path_impl` / substring all populate `backing` via `rock_longlived_alloc` (Phase H step 2 + 3).
- [x] Length-aware output paths in place (Phase H step 4).
- [x] Capacity-gated setCharAt (Phase H step 5).
- [x] Substring view + alias-via-retain validated by `test/substring_view_test.rkr` and `test/string_alias_test.rkr`.

Implementation note (2026-04-29): tight-loop allocation pressure validated by `test/string_alloc_loop_test.rkr` (5000 iterations in default longlived pool). ZXN smoke compile of `simple_test` and `format_test` verifies SDCC compatibility of `pools.c` and the new `rock_longlived_alloc` paths.

## Phase I: Arrays and aggregate operations

- [ ] Remove `get(arr, i)` and `set(arr, i, v)` builtins.
- [ ] Generate bracket read and write forms.
- [ ] Implement `remove(arr, i)`.
- [ ] Implement `compact(arr)`.
- [ ] Ensure `append`, `insert`, assignment, `pop`, and `remove` preserve descriptor/handle copy semantics.
- [ ] Ensure string arrays no longer use `is_string_array` or special deep-copy paths.

Phase exit checks:

- [ ] `test/string_array_shallow_test.rkr` passes.
- [ ] `test/array_remove_test.rkr` passes.
- [ ] `test/array_compact_test.rkr` passes.
- [ ] Existing migrated array tests pass.

## Phase J: Builtin typing and dead-code removal

- [ ] Convert remaining `register_builtin` calls to `register_builtin_typed`.
- [ ] Delete `__free_string`.
- [ ] Delete `TRACK_STRING`.
- [ ] Delete `is_string_array`.
- [ ] Delete `track_string_var` and `track_string_tmp`.
- [ ] Delete `emit_return_cleanup`.
- [ ] Delete string-specific deep-copy paths in record construction.

Phase exit checks:

- [ ] ADR-0003 §16.3 structural greps pass.
- [ ] Full compiler build has no dead-code warnings.

## Phase K: Diagnostics and explainability

- [ ] Implement `--explain-allocations`.
- [ ] Report store promotions with file, line, kind, and destination.
- [ ] Report return materialisation sites separately from store promotions.
- [ ] Add debug-build region accounting.
- [ ] Add debug-build alloc-site tagging for longlived blocks.
- [ ] Add debug-build fragmentation watchdog.
- [ ] Add debug-build leak report at program exit.

Phase exit checks:

- [ ] Diagnostics are disabled in release builds.
- [ ] Debug leak detector catches an intentionally leaked synthetic case.
- [ ] Allocation explanations distinguish statement scratch, destination-driven allocation, store promotion, and return materialisation.

## Phase L: Test migration and target validation

- [ ] Migrate all existing `get(arr, i)` calls to `arr[i]`.
- [ ] Migrate all existing `set(arr, i, v)` calls to `arr[i] := v`.
- [ ] Migrate `new_string` calls to `clone`.
- [ ] Add capacities to longlived arrays that require them.
- [ ] Add every new behavioural test listed in ADR-0003 §16.2.
- [ ] Run full host suite.
- [ ] Run full host suite under `--memory-profile=zxn`.
- [ ] Run ZXN smoke tests.

Phase exit checks:

- [ ] Zero functional regressions across migrated tests.
- [ ] ZXN smoke tests produce `.nex` files and run without runtime fault.

## Phase M: Wiki closeout

- [ ] Rewrite [[concepts/string-representation]].
- [ ] Rewrite [[concepts/array-internals]].
- [ ] Update [[syntax/strings]].
- [ ] Update [[syntax/arrays]].
- [ ] Update [[syntax/builtins-and-io]].
- [ ] Update [[syntax/syntax-index]].
- [ ] Mark [[decisions/ADR-0002-string-view-memory-model]] as superseded.
- [ ] Update this implementation plan with final implementation lessons.
- [ ] Promote [[decisions/ADR-0003-memory-model]] from `draft` to `accepted`.

Phase exit checks:

- [ ] Wiki does not document old `owned`, `new_string`, `get`/`set`, `is_string_array`, or hidden caller result-region behaviour as current.
- [ ] Wiki pages and code agree on return materialisation, bump release, loop-carried slots, and string backing.

## Implementation lesson log

Append short notes here while implementing. If a note changes design, update ADR-0003.

| Date | Phase | Lesson | Follow-up |
|------|-------|--------|-----------|
| 2026-04-28 | planning | Execution needs a mutable checklist separate from the ADR design contract. | Created this page and linked it from ADR-0003. |
