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
- [ ] Implement `__pool_bump_alloc`, `__pool_bump_save`, and `__pool_bump_restore`.
- [ ] Implement `__pool_longlived_alloc` and `__pool_longlived_free` with universal `{size, refcount}` headers.
- [ ] Implement free-block and static-block sentinels.
- [ ] Implement size-class freelists and miscellaneous-class handling.
- [ ] Implement `reclaim()` as non-relocating adjacent-free-block coalescing.
- [ ] Add named OOM diagnostics for `bump` and `longlived`.
- [ ] Implement `--memory-profile=zxn` for host builds.

Phase exit checks:

- [ ] Pool unit tests or focused Rock tests can exhaust each pool with named diagnostics.
- [ ] `reclaim()` tests show adjacent free blocks coalesce and non-adjacent blocks do not.
- [ ] Host `--memory-profile=zxn` uses the same pool sizes as the ZXN manifest.

## Phase B: Typechecker acyclicity

- [ ] Build the user-type graph after type resolution.
- [ ] Walk through array, record, union, and descriptor fields.
- [ ] Reject direct recursive types.
- [ ] Reject indirect recursive cycles.
- [ ] Emit a diagnostic that names the cycle path and recommends flat index-based structures.

Phase exit checks:

- [ ] `test/recursive_type_rejected_test.rkr` exists and passes.
- [ ] Non-recursive aggregate combinations still typecheck.

## Phase C: Region emission and cleanup skeleton

- [ ] Emit bump save/restore for function bodies.
- [ ] Emit bump save/restore for `if`, `else`, loop, and `case` bodies.
- [ ] Emit fresh region handling for each loop iteration.
- [ ] Emit statement-region save/restore.
- [ ] Add freestanding plain-block statement parsing.
- [ ] Build compile-time cleanup records with typed owned locals.
- [ ] Replace return-only cleanup with `emit_unwind_to(target_scope)`.

Phase exit checks:

- [ ] `test/region_iteration_test.rkr` passes.
- [ ] `test/region_nested_block_test.rkr` passes.
- [ ] Generated code for early return restores the correct bump marks in order.

## Phase D: Descriptor and handle layout

- [ ] Replace `struct string.owned` with `capacity` and `backing`.
- [ ] Add universal block header access helpers.
- [ ] Ensure arrays do not carry a second refcount in `__internal_dynamic_array_t`.
- [ ] Ensure aggregates do not introduce a separate `__aggregate_header`.
- [ ] Update generated allocations to initialise `capacity`, `backing`, and universal-header refcounts.
- [ ] Emit static string literal backing headers with `refcount = 0xFFFF`.

Phase exit checks:

- [ ] Structural grep shows no live `owned` field usage.
- [ ] Static literal descriptors point at sentinel backing.
- [ ] Bump string descriptors use `backing = NULL`.

## Phase E: Retain/release runtime and generated walkers

- [ ] Implement `__string_retain` and `__string_release`.
- [ ] Implement array retain/release using universal headers.
- [ ] Implement aggregate retain/release using universal headers.
- [ ] Generate `__retain_T` and `__release_T` for every used record, union, and module type.
- [ ] Generate `__release_array_T` for every used array element type.
- [ ] Ensure bump-container dec-to-zero releases children but does not freelist-free physical storage.
- [ ] Ensure longlived-container dec-to-zero releases children, then frees physical storage.
- [ ] Emit retain/release on assignment, overwrite, slot write, field write, scope exit, and discarded producer temporaries.
- [ ] Emit callee-side retain/release for every refcounted parameter.

Phase exit checks:

- [ ] `test/string_backing_refcount_test.rkr` passes.
- [ ] `test/string_substring_retain_test.rkr` passes.
- [ ] `test/record_with_string_release_test.rkr` passes.
- [ ] `test/array_of_records_with_strings_test.rkr` passes.
- [ ] `test/bump_alias_release_test.rkr` passes.

## Phase F: Return materialisation

- [ ] Implement `__return_string`.
- [ ] Generate `__return_T` for every returned record, union, and module type.
- [ ] Generate `__return_array_T` for every returned array element type.
- [ ] Ensure static returns are unchanged.
- [ ] Ensure longlived non-static returns retain once for the caller.
- [ ] Ensure bump-backed returns allocate-copy into `longlived`.
- [ ] Ensure callee unwind releases the original temp/local/parameter references after `__return_T`.
- [ ] Ensure caller capture treats every non-scalar function result as a producer transfer.
- [ ] Remove any hidden caller `__result_region` function ABI code path.

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

- [ ] Make `substring` return a view with inherited backing.
- [ ] Make `charAt`, `equals`, and `print` length-aware.
- [ ] Replace generated C `printf("%s", expr.data)` for Rock strings.
- [ ] Update file output to use `fwrite(s.data, 1, s.length, f)`.
- [ ] Implement capacity-gated `setCharAt`.
- [ ] Replace `new_string` with `clone`.
- [ ] Keep one `string` type; do not add `stringbuf` or `char[]` string-like duplicates.

Phase exit checks:

- [ ] `test/string_view_test.rkr` passes.
- [ ] `test/string_capacity_test.rkr` passes.
- [ ] `test/string_alias_mutation_test.rkr` passes.
- [ ] `test/string_clone_test.rkr` passes.

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
