---
title: "ADR-0003: Memory model — named pools, block-scoped regions, descriptor values"
category: decisions
tags: [adr, memory, regions, pools, refcount, strings, arrays, runtime, zxn, escape-analysis]
sources:
  - src/lib/typedefs.h
  - src/lib/alloc.c
  - src/lib/alloc.h
  - src/lib/fundefs.c
  - src/lib/fundefs.h
  - src/lib/fundefs_internal.c
  - src/lib/fundefs_internal.h
  - src/lib/zxn/zpragma_zxn.inc
  - src/generator.c
  - src/generator.h
  - src/name_table.c
  - src/typechecker.c
updated: 2026-04-28
status: draft
---

# ADR-0003: Memory model — named pools, block-scoped regions, descriptor values

**Status:** Draft (2026-04-28). Supersedes [[decisions/ADR-0002-string-view-memory-model]].

This ADR specifies a complete redesign of Rock's memory model. It is intended to be implemented as one atomic change with no partial-migration states, no compatibility flags, and no follow-up TODOs. The scope covers strings, arrays, records, unions, modules, the runtime allocator, the per-target memory layout, the generator's escape analysis, and the public Rock-facing API.

---

## 1. Priorities

The decisions below are driven by these priorities, in this order. Conflicts are resolved in favour of **lower**-numbered items (priority 1 outranks priority 5).

1. **No silent memory leakage.** Every allocation has a known reclamation point. The runtime is provably leak-free for any program that compiles cleanly.
2. **Predictability on tiny systems.** Memory layout is fixed. Pool boundaries do not move. Out-of-memory failures are per-pool, named, and informative. Programs targeting ~32 KB systems can reason about memory at a sub-pool granularity.
3. **Orthogonality.** One ownership rule applies uniformly to strings, arrays, records, unions, and modules. No type carries a special-case ownership story.
4. **Ergonomic source.** The compiler does the bookkeeping the user would otherwise type out. User-visible allocation primitives exist only where deliberate user control is required.
5. **Implementation effort is acceptable cost.** The plan specifies a single atomic implementation. Partial degradation, compatibility shims, and post-merge follow-ups are explicitly disallowed.

## 2. Assumptions

Stated explicitly so future reviewers can challenge them rather than infer them from omission.

- **Target envelope.** Rock targets 8-bit systems with roughly 24 KB to 40 KB of usable RAM (e.g. ZX Spectrum Next when sharing RAM with a ROM-resident environment). The host (gcc) target is a generous superset and does not constrain the runtime design; it follows the same model with larger default pool sizes.
- **No threading, no preemption.** Rock has no threads, async, or interrupts that interact with allocator state. All allocator state is single-owner.
- **No first-class functions.** Rock has no closures, no user-visible function pointers held across function boundaries, and no callbacks stored into long-lived state.
- **Acyclic value graph (enforced).** Rock's record, array, and union types must form a structurally acyclic field graph. The typechecker rejects any user-defined type whose field graph, walked transitively through scalars, descriptors, and handles, contains its own type as a reachable node. This is enforced at compile time (§9.4); the assumption is the **invariant the compiler establishes**, not a property the user must remember. The cost is a deliberate language limitation: trees and graphs cannot be expressed as recursive type definitions and must instead use flat collections with index references. See §15.
- **Cycle-freedom at runtime follows from cycle-freedom at type level.** Because no field can refer back to its containing type, no value graph at runtime can form a cycle. Refcount-based reclamation (§7, §8) is therefore complete.
- **Bank/MMU layout for ZXN is target-managed.** The compiler-emitted runtime does not perform MMU bank switching. Pool placements respect MMU slot granularity so they fit within addressable banks under the target's default paging.
- **SDCC and gcc both accept `char *` (mutable) as the storage type for string backing.** A future tightening to `const char *` is desirable but is not blocked by this ADR.

## 3. Inferences

Stated as inferences so the chain of reasoning is auditable.

- **A growing-boundary heap is not viable.** The compiler shares physical address space with raw data blocks, inline assembly, sprite buffers, and screen memory. These need fixed addresses. A bump-down/freelist-up arrangement that meets in the middle leaves no contiguous user-reserved range. Therefore each pool must have fixed start and size, declared at compile time.
- **Stack-disciplined block scopes cannot fragment.** Block scopes nest syntactically: an outer block cannot close while an inner block is open. A bump pointer that is saved on block entry and restored on block exit is therefore mathematically incapable of fragmenting the bump pool. Reclamation is O(1) per block and complete.
- **Refcount on aggregate headers and string backing is sufficient given enforced acyclicity.** Because the typechecker establishes the acyclic-value-graph invariant, every allocation's reference graph at runtime is a DAG. A refcount that tracks direct strong references reaches zero exactly when the allocation becomes unreachable. This applies to array headers, record/union/module bodies, and **long-lived string backing buffers** (§7.6).
- **Implicit promotion is deterministic.** The lifetime relation between any two values can be computed from the syntax tree (each value's enclosing scope determines its region; deeper scopes are shorter-lived). The relation is a partial order — sibling scopes are incomparable but mutually unreachable — so the compiler can decide for every store whether promotion is required. The user does not need to write `clone(...)` calls to make sound code.
- **Value relocation requires handle indirection.** Compacting a heap by moving live blocks would require every reference to that block to be updated. The only practical way to do that on a Z80 is to introduce a handle table (id → real pointer), adding one indirection on every dereference. The cost in code size and cycles outweighs the fragmentation it would address. **This ADR rejects relocating compaction.**

## 4. Memory layout: named pools at fixed addresses

The runtime presents two compiler-managed pools, each with fixed start address and fixed size, declared in a per-target memory manifest. Pools never move, never grow, never share boundaries.

### 4.1 Pool inventory

| Pool        | Allocator                                       | Reclamation point                                    |
|-------------|-------------------------------------------------|------------------------------------------------------|
| `bump`      | Single bump pointer; save/restore per block     | Block exit — pointer restore                         |
| `longlived` | Size-class freelist + refcount on block headers | Refcount drop to zero; explicit `compact(arr)`       |

### 4.2 Non-pool regions

The remainder of the address space is **not** managed by the Rock runtime and is reserved for the listed purposes. The compiler emits no allocation into these ranges.

| Region            | Purpose                                              |
|-------------------|------------------------------------------------------|
| Static / code     | Program code, rodata, string literals (linker-managed) |
| User-reserved     | Sprite patterns, tilemap data, raw `byte[]` buffers loaded from disk, inline `@embed asm` targets |
| Stack             | Z80 hardware call stack                              |
| Memory-mapped I/O | Hardware register windows accessed via `peek`/`poke` |

The `user-reserved` range is the contractual answer to the requirement that mixed assembly and raw data have stable, predictable addresses. The runtime guarantees never to allocate into it.

### 4.3 Per-target memory manifest

Pool placements are target-specific configuration, not language semantics. The manifest is read at compile time and the resulting bounds are embedded into the generated runtime initialiser.

#### ZXN default layout

Default sizes are subject to measurement during the foundation port. The manifest is the authoritative source; the layout below is illustrative.

```
$0000–$3FFF   ROM                    (paged, target-controlled)
$4000–$5AFF   Display + attributes   (5,888 bytes)
$5B00–$7FFF   Free / RTL state       (target-reserved)
$8000–$BFFF   Code                   (16 KB; z88dk text + rodata)
$C000–$DBFF   Pool: bump             (7,168 bytes)
$DC00–$E7FF   Pool: longlived        (3,072 bytes)
$E800–$FEFF   User-reserved          (5,888 bytes)
$FF00–$FFFF   Stack                  (256 bytes)
```

#### Manifest format

The manifest lives alongside the existing `src/lib/zxn/zpragma_zxn.inc` and follows the same per-target convention.

```
pool bump        origin=$C000  size=$1C00  alloc=bump
pool longlived   origin=$DC00  size=$0C00  alloc=freelist
reserved code        $8000–$BFFF
reserved user        $E800–$FEFF
```

#### Host (gcc) layout

The host target uses `malloc`'d backing for each pool at startup with generous defaults (e.g. 4 MB each). The API surface is identical to the ZXN target; only the manifest differs. This guarantees that programs which run cleanly on the host have an immediately portable memory profile to the ZXN target — only pool sizing, not pool semantics, varies.

### 4.4 Per-program override

Programs may override pool sizes via a source pragma when their intended workload diverges from the default split (e.g. a graphics-heavy program that shrinks `longlived` to expand the user-reserved range):

```rock
@pragma pool bump      size=$1400
@pragma pool longlived size=$1400
@pragma pool user      size=$2400
```

The pragma is parsed at compile time; the generator emits the resulting bounds into the runtime initialiser. Pragmas do not change pool semantics, only sizes.

### 4.5 Out-of-memory diagnostics

OOM is per-pool. The runtime emits one of:

```
runtime: bump pool exhausted at $D7E0
  requested: 24 bytes  available: 16 bytes
  allocator stack: <function name> > <inner block>
runtime: longlived pool exhausted
  requested: 64 bytes  largest free: 32 bytes
  fragmentation: 38%  (largest free / total free)
```

The `bump` pool exhaustion is recoverable only by exiting one or more enclosing blocks. The `longlived` pool exhaustion is recoverable by `compact(arr)`-driven trim or by application-level reclamation. The runtime never guesses at recovery; it halts.

## 5. Block scopes are regions

There is no `region` keyword. Every Rock block (`{ ... }`) defines a region. Region nesting equals scope nesting; lifetime ordering equals scope-tree depth.

### 5.1 What constitutes a block

| Block form                          | Region behaviour                                       |
|-------------------------------------|--------------------------------------------------------|
| Function body                       | Outermost region for the call                          |
| `if` arm body                       | Region; resets on arm exit                             |
| `else` / `else if` arm body         | Region; resets on arm exit                             |
| `while` body                        | **Fresh region per iteration**; resets at iteration end |
| `for ... to ...` body               | Fresh region per iteration                             |
| `for x in arr` body                 | Fresh region per iteration                             |
| `case` arm body                     | Region; resets on arm exit                             |
| Plain `{ ... }` as a statement      | Region; resets at brace close                          |

The plain-block-as-statement form must be admitted by the parser as a freestanding `Statement` production so users can introduce nested regions for memory scoping without a control-flow excuse.

### 5.2 Mechanics

The `bump` pool has a single global bump pointer `B`. On block entry the compiler emits a save of `B` into a slot in the calling frame; on block exit it emits a restore. Allocations within the block grow `B` upward (toward the pool's high address). Restore is two instructions on Z80: load saved value, store back.

```text
Function entry:               B = B0
  alloc x (10 bytes):         B = B0 + 10
  enter while body:           save B as W0
    iter 1:
      alloc tmp:              B = W0 + 60
    iter end:                 B = W0
    iter 2:
      alloc tmp:              B = W0 + 30
    iter end:                 B = W0
  exit while:                 (B already W0)
  alloc y:                    B = W0 + 8
Function exit:                B = B0
```

Because block scopes are LIFO-nested, the saved-restore-pair sequence is well-formed by construction.

### 5.3 Statements are also regions

Each statement is a region in the same sense as a block: the generator emits a bump-pointer save before the statement and a restore after it. Allocations made for expression temporaries that do not escape the statement die at the semicolon.

For declarations like `string s := concat(a, b)`, the assignment's RHS is computed in the statement's region by default, but the compiler applies **destination-driven allocation** (§10.5): the result region of `concat` is set to `s`'s enclosing block region, so the backing is allocated directly into the block. No statement-scratch allocation, no copy at the semicolon.

For pure expression-statements like `print(toString(n))`, the `toString` result has no destination outside the statement; it is allocated in statement scratch and reclaimed at the semicolon. This keeps bump-pool pressure bounded in straight-line code without requiring user syntax.

The granularity is therefore: statement < block < function-body < `longlived` < static. All are regions; all use the same save/restore mechanism on the bump pool.

### 5.4 No user-visible region keyword

Region introduction, region close, and region reset are all consequences of block syntax. There is no `region`, no `arena`, no `scope` keyword. The user manages memory by managing scope.

## 6. Value model: descriptors and handles

All Rock values either are immediate scalars or are descriptors/handles into one of the pools.

| Rock shape          | Storage                                                      | Copy semantics                       |
|---------------------|--------------------------------------------------------------|--------------------------------------|
| `int`, `byte`, `word`, `dword`, `bool`, `char`, `float` | Immediate value     | Value copy                          |
| `string`            | `{data, length, capacity, backing}` descriptor (8 bytes on Z80) | Descriptor copy; backing is shared; refcount inc/dec on backing if non-static |
| `T[]`               | Pointer to header (`__internal_dynamic_array_t`) which references the element buffer | Handle copy; refcount inc/dec |
| `record`            | Pointer to header + body                                     | Handle copy; refcount inc/dec        |
| `union`             | Pointer to header + body (which contains key + value)        | Handle copy; refcount inc/dec        |
| `module`            | Pointer to module struct (singleton; refcount irrelevant)    | Handle copy                          |

Assignment, parameter passing, return, record-field write, and array-slot write all perform descriptor or handle copy. None deep-copy. Backing memory is owned by its pool; descriptors and handles do not own their target.

This is the single ownership rule. There are no type-specific exceptions.

## 7. Strings

### 7.1 Layout

```c
typedef struct string {
  char            *data;     // logical start; for views may be offset into backing
  size_t           length;
  size_t           capacity; // 0 = read-only view; >0 = writable backing
  __string_block  *backing;  // header for the backing block; NULL means bump
} string;
```

`data` is `char *` rather than `const char *` to permit in-place mutation when `capacity > 0`. Const-correctness for views is enforced by the runtime check on `capacity`, not by the type of the pointer.

The `backing` field distinguishes three lifetime classes for the underlying bytes — one of the central correctness contracts of this ADR. The discriminant is the combination of `backing` and the header's refcount:

| `backing`   | `header->refcount` | Meaning   | retain/release | Promotion when stored to longer-lived destination |
|-------------|--------------------|-----------|----------------|-----------------------------------------------|
| `NULL`      | n/a                | bump      | no-op          | **required** (allocate longlived block, copy bytes) |
| non-`NULL`  | `0xFFFF` (sentinel)| static    | no-op          | not required (eternal lifetime)               |
| non-`NULL`  | `< 0xFFFF`          | longlived | inc / dec      | not required                                  |

The `__string_block` header is the universal block header (§8.5) — every refcount-managed block in the system uses the same layout, regardless of whether the payload is string bytes, an array's `__internal_dynamic_array_t`, or a record/union/module body. There is one canonical refcount location: the universal block header.

```c
typedef struct __string_block {     // a longlived block whose payload is string bytes
  uint16_t size;       // size in bytes of the bytes payload
  uint16_t refcount;   // 0xFFFF = static; 0xFFFE = free; otherwise normal refcount
  // bytes follow (length is stored in the descriptor, not in the block)
} __string_block;
```

For refcounted strings, the descriptor's `data` normally points just past the header. For substrings of refcounted backing (including substrings of static literals), `data` is offset into the source's bytes but `backing` still points at the source's header, so retain/release and promotion checks operate on the source's lifetime correctly.

#### String literal emission

Every string literal in the program emits a static `__string_block` in rodata with `refcount = 0xFFFF`, plus a static descriptor whose `backing` points at it. Cost: 2 bytes per literal in static rodata for the sentinel header. A program with 50 literals adds ~100 bytes of static data.

#### Promotion check at runtime

```c
// Conceptual; the actual call is generated per-site
bool needs_promotion(string s, region_t dst) {
  if (s.backing == NULL)                       return true;   // bump → must promote to longlived
  if (s.backing->refcount == 0xFFFF)           return false;  // static → eternal
  /* longlived */                              return false;  // already lives in longlived
}
```

Two pointer dereferences. ~10-20 cycles per check on Z80. Affordable.

### 7.2 Capacity and backing semantics

| Source                                        | `capacity` | `backing`             | Notes                                                                        |
|-----------------------------------------------|------------|-----------------------|------------------------------------------------------------------------------|
| String literal                                | `0`        | static sentinel header| Compile-time-emitted `__string_block` in rodata with `refcount = 0xFFFF`      |
| `substring(literal, ...)`                     | `0`        | inherited (literal's static sentinel) | View into static; retain/release no-op via sentinel check    |
| `substring(bump_str, ...)`                    | `0`        | `NULL` (inherited from bump source) | View into bump; lives with bump                                |
| `substring(longlived_str, ...)`               | `0`        | inherited (source's longlived header) | View; retain increments source backing's refcount             |
| `concat(a, b)`                                | `length`   | new longlived block   | Allocates new `__string_block` in destination region (§10)                    |
| `toString(n)`                                 | `length`   | new longlived block   | Allocates new `__string_block` in destination region                          |
| `clone(s)`                                    | `length`   | new longlived block   | Allocates new `__string_block` in destination region                          |
| `read_file(path)`                             | `length`   | new longlived block   | Allocates new `__string_block` in destination region                          |
| Promotion bump → longlived                    | `length`   | new longlived block   | Compiler-emitted at the store site                                            |

A capacity of `0` means **read-only**: `setCharAt` on such a string halts with a runtime diagnostic. A capacity greater than zero means writable in place, up to `capacity` bytes (which equals `length` for current operations; in-place growth is not supported).

### 7.3 Mutation semantics

Mutation of a `capacity > 0` string is observable through any alias of the same descriptor. This is the same rule that governs arrays and records: aliases share backing, and mutation is visible.

```rock
string s := clone("hello");   // capacity = 5
string t := s;                // descriptor copy; t and s share backing
setCharAt(t, 0, 'H');         // s and t both observe "Hello"
```

To mutate without affecting other aliases, clone first:

```rock
string u := clone(s);
setCharAt(u, 0, 'X');         // s unaffected
```

This is the **local mutation contract**: mutation is permitted, and its visibility through aliases is defined and predictable.

### 7.4 Forbidden in-place operations

In-place growth (extending `length` beyond `capacity`) is not supported. Length is fixed at the moment a writable string is allocated. To produce a longer string, use `concat` and accept the fresh allocation. Syntactic sugar over `concat` may be added later if measurement justifies it; nothing in this ADR depends on it.

### 7.5 String API after the change

| Operation                          | Behaviour                                              | Allocates? |
|------------------------------------|--------------------------------------------------------|------------|
| `length(s)`                        | Returns `s.length`                                     | No         |
| `charAt(s, i)`                     | Returns `s.data[i]`; bounds-checked                    | No         |
| `setCharAt(s, i, c)`               | Requires `s.capacity > 0`; writes `s.data[i] = c`       | No         |
| `equals(a, b)`                     | Length + memcmp over `length` bytes                    | No         |
| `substring(s, from)`               | Returns view: `{s.data + from, s.length - from, 0}`    | No         |
| `substring(s, from, end)`          | Returns view: `{s.data + from, end - from + 1, 0}`     | No         |
| `concat(a, b)`                     | Allocates new buffer in destination region; writes a then b | Yes   |
| `toString(n)`                      | Allocates and formats into destination region          | Yes        |
| `clone(s)`                         | Allocates and copies `length` bytes                    | Yes        |
| `print(s)`                         | Length-aware loop                                      | No         |
| `write_string_to_file(s, path)`    | `fwrite(s.data, 1, s.length, f)`                       | No         |

The earlier `string_to_cstr` and `new_string` are removed (see §13).

### 7.6 Refcount on string backing

Long-lived string backing reclamation parallels arrays:

- The compiler emits `__string_retain(s)` and `__string_release(s)` at every site that creates or destroys a strong descriptor reference. The retain/release helpers no-op when `s.backing == NULL`; otherwise they inc/dec the 16-bit refcount in the `__string_block` header.
- When refcount reaches zero, the block returns to its size-class freelist in `longlived`.
- All sites listed in §8.2 (assignment, slot write, scope exit, etc.) apply identically to strings. Strings, arrays, and aggregates share one retain/release rule set.

#### Sites where retain/release fires for strings

The full coverage matrix; the generator must emit at every entry:

The action depends on whether the source expression is a **producer** (fresh allocation: `concat`, `toString`, `clone`, `read_file`, returning function with non-scalar type, array literal, constructor call) or a **borrower** (variable read, field/slot read). See §10.6 for the producer/borrower convention in full.

| Site                                                                | Source kind | Action                                  |
|---------------------------------------------------------------------|-------------|-----------------------------------------|
| Local variable initialiser `string s := producer_expr`              | producer    | **transfer** (no inc; producer's rc=1 becomes s's) |
| Local variable initialiser `string s := borrower_expr`              | borrower    | retain RHS                              |
| Local reassignment `s := producer_expr`                             | producer    | release old s; transfer (no inc)        |
| Local reassignment `s := borrower_expr`                             | borrower    | release old s; retain new               |
| Record/union/module field write, slot write, etc.                   | producer    | release old; transfer                   |
| Record/union/module field write, slot write, etc.                   | borrower    | release old; retain                     |
| `append(arr, producer_expr)` / `insert(arr, i, producer_expr)`      | producer    | transfer into slot (no inc)             |
| `append(arr, borrower_expr)` / `insert(arr, i, borrower_expr)`      | borrower    | retain                                  |
| `pop(arr)` / `remove(arr, i)`                                       | producer (transfer-out) | no inc; caller's destination transfer   |
| Discarded refcounted temporary at statement boundary                | producer    | release at semicolon                    |
| Function parameter pass (caller side)                               | either      | nothing                                  |
| Function entry (callee side)                                        | n/a         | **retain every refcounted parameter**   |
| Function exit (callee side)                                         | n/a         | **release every refcounted parameter**  |
| Function return (callee side)                                       | n/a         | call `__return_string(retval)`; see §10.3 |
| Function return (caller side, capturing the result)                 | producer    | transfer (the returned result is now caller-owned) |
| Scope exit                                                           | n/a         | release every owned local in scope      |
| Generated `__release_T` walks string-typed fields of T              | n/a         | calls `__string_release` on each before freeing T |
| Generated `__release_array_string` walks string slots               | n/a         | calls `__string_release` on each before freeing the buffer |

#### Parameter ABI: callee retains on entry, releases on exit

Earlier drafts of this ADR used true borrow semantics (no inc/dec at the call boundary). That was unsound: a callee can release the caller's only strong reference (e.g. `f(s) { global := other; use(s); }` where `s` came from `global`) and then continue using its borrowed parameter, which now points at freed memory.

The fix is the standard ARC-style ABI. Every refcounted parameter is incremented on function entry and decremented on function exit. Cost: ~50 cycles per parameter per call; no extra storage (the parameter handle is already on hand). The callee can freely overwrite caller-side slots without invalidating its own parameters because the parameter's refcount is stable for the call duration.

A future borrow-safety analysis pass could elide the retain/release pair for functions that provably do not write any refcounted slot reachable from outside their own local scopes. This is an optimisation, not a correctness fix; deferred to §19.

#### Return ABI: callee materialises returns in longlived as an owned producer

A non-scalar return crosses a function-call boundary: the callee's bump frame is reclaimed at unwind, so any value whose backing was in the callee's bump cannot be returned by descriptor alone. The callee must produce a value whose backing outlives the unwind. The only pool that satisfies this without colliding with the single bump-pointer's LIFO discipline is `longlived`.

**Rule:** every non-scalar return is materialised in `longlived` by the callee before unwinding, and the returned descriptor/handle is an owned producer. Return materialisation is deliberately separate from ordinary store promotion: `__promote_T(value, region)` makes a value lifetime-safe for a destination, while `__return_T(value)` creates an owned result for the caller.

```c
// Conceptual; the actual emission is per-type
__return_string(s):
  if s.backing == NULL:                              // bump source
    allocate new __string_block in longlived; copy bytes; rc = 1
    return new descriptor pointing at it
  if s.backing->refcount == STATIC_SENTINEL:         // static source
    return s unchanged                               // eternal; caller transfer is sound (no inc needed)
  // longlived source
  s.backing->refcount += 1                           // inc to give caller an owned reference
  return s
```

Return materialisation always produces "+1 owned" for longlived non-static sources and fresh-rc-1 for bump sources. The static path is the only one that returns without an inc (because static is eternal and the caller's "transfer" rule never decrements a static block).

For every non-scalar `return expr`, the callee:

1. evaluates `expr` into a temporary;
2. calls `__return_T(temp)`;
3. unwinds normally, releasing the original temp/local/parameter references;
4. returns the owned result from `__return_T`.

Specifically, for the various return cases:

- **Returning a static literal**: descriptor returned unchanged; backing is the static sentinel header; caller transfer is sound because static has no refcount semantics.
- **Returning a longlived value** (parameter, local, field-read): `__return_T` inc's the longlived block's refcount; returns the same descriptor/handle. The callee's parameter/local/temp release on unwind decrements the callee-owned reference; the return inc survives for the caller.
- **Returning a bump-allocated local**: `__return_T` allocates a fresh longlived block, copies, returns a new descriptor/handle pointing at it. The bump source dies with callee unwind; the longlived copy survives.
- **Returning a freshly-allocated longlived producer** (the result of `concat`/`toString`/`clone`/etc. invoked inside this function): `__return_T` still retains the value, then normal unwind releases the callee's producer temporary. Net result: caller receives rc=1. A future optimisation may consume/move a proven-fresh producer to avoid the retain/release pair, but v1 does not rely on that optimisation.

Caller capture is uniform: the returned descriptor is a producer; the caller transfers it to the destination slot (no inc, no dec). Producer/borrower classification (§10.6) treats every non-scalar function call result as a producer.

#### No caller result-region parameter is part of the ABI

Earlier drafts threaded a hidden caller result-region parameter from caller to callee so that the callee could allocate into the caller's destination region. That model was unsound under a single LIFO bump pointer: a caller-side bump destination is below the callee's frame, and any allocation the callee makes during its execution is above the callee's entry mark, so callee unwind reclaims it. There is no implementation that satisfies the model as written without either multiple bump arenas (rejected on complexity grounds) or per-call result memmove (rejected on cycle-cost grounds for variable-sized returns).

The simpler and sound rule is the one above: callee allocates returns in `longlived`. There is no hidden region parameter. Caller-side destination region does not affect the call ABI.

The same rule applies to arrays, records, and unions returned from functions: they materialise in `longlived` via per-type generated `__return_T(handle)` walkers.

## 8. Arrays

### 8.1 Layout

```c
typedef struct __internal_dynamic_array_t {
  void   *data;              // element buffer, in same pool as the header that wraps this struct
  size_t  length;
  size_t  capacity;
  size_t  elem_size;
  size_t  max_capacity;      // 0 = unset (only valid in bump pool); >0 = fixed cap
} __internal_dynamic_array_t;
```

The array value carried in Rock variables is a **handle** — a pointer to this struct. The struct itself is the payload of a universal block header (§8.5); the refcount lives in that header, not in the struct. Given a handle `h`, the header is at address `((__longlived_header *)h) - 1` (i.e. immediately preceding the struct in memory).

Aliases share the handle, the wrapping header (and therefore the refcount), and the element buffer.

### 8.2 Refcount

A 16-bit refcount lives in the header. It tracks the number of strong references (Rock variables, array slots, record fields) currently pointing at this header.

- **Fresh allocations start at refcount = 1.** The producing expression owns the initial reference. See §10.6 for the producer / borrower / transfer convention that governs how that reference travels into destinations.
- **Transfer** (no inc, no dec) when the source is a producer expression and the destination is a stable slot. The producer's refcount = 1 becomes the destination's.
- **Retain** (inc) when the source is a borrower expression (existing slot read) and the destination is another stable slot.
- **Release** on every overwrite of a slot that previously held a strong reference, on scope exit for owned locals, and on statement boundary for discarded refcounted temporaries.
- **Function parameter ABI**: callee retains every refcounted parameter on entry and releases on exit. This is the unconditional rule; the borrow-safety optimisation is deferred to §19.
- **Function return ABI**: callee materialises every non-scalar return in `longlived` via `__return_T` before unwinding. The return helper retain-and-returns longlived non-static sources, allocates+copies bump sources, and returns static sources unchanged. The caller's capture is a transfer from a producer (no further inc/dec). There is no hidden caller result-region parameter. See §7.6 for the full soundness rationale.
- When refcount reaches zero: walk every element of type T using the generated `__release_T` (so that string fields, child arrays, and child records are released first); then the element buffer returns to its size-class freelist; then the header returns to the header freelist.
- **Reads** (indexing, `length`, iteration) do not touch refcount.

### 8.3 Pool selection for arrays

Where an array's wrapping header and element buffer are allocated depends on the array's lifetime:

- **Bump pool**: arrays whose handle does not escape the enclosing block AND whose declaring slot is not loop-carried (§11.8). Header and element buffer reside in bump.
- **`longlived` pool**: arrays whose handle escapes the enclosing block, OR whose declaring slot is written from inside a loop body whose body is a descendant of the slot's declaring scope (loop-carried; §11.8). Header and element buffer are size-class freelist allocations.

The compiler chooses the pool based on escape and lifetime analysis (§11). The user does not name a pool.

#### Refcount applies in both pools

Refcount is set to 1 on creation in either pool. It is incremented on every aliasing assignment and decremented on overwrite/scope exit, regardless of pool — same-block aliasing of a bump array is real and must be tracked so that nested longlived children (e.g. string descriptors stored in a `string[]`) are released exactly once.

On dec-to-zero, the per-type `__release_T` walker runs to release every refcounted child. After the walk:

- **If the block lives in the longlived pool**: the header and element buffer return to their size-class freelists.
- **If the block lives in the bump pool**: the storage is *not* returned to a freelist. The bump pointer will reclaim it at the enclosing block's exit. The dec-to-zero work is therefore "release children, do not free physical storage."

The bump-vs-longlived check at release is a single address-range comparison against the bump pool bounds (~10 cycles on Z80).

### 8.4 Fixed-capacity in long-lived arrays

Dynamic growth of array element buffers in the `longlived` pool is **not supported**. Any array allocated in `longlived` must have an explicit capacity at allocation time. Expressions of the form

```rock
int[] xs := [];   // empty dynamic array
append(xs, value);  // would require growth
```

are valid only when the resulting handle does not escape the block (and therefore lives in `bump`, where growth is acceptable because the entire pool is reset on block exit).

When the user wants a long-lived growing array, the language requires an explicit capacity:

```rock
int[N] xs;       // fixed capacity N
append(xs, value);
```

The compiler rejects, with a clear diagnostic, any uncapacitied array that escapes its declaring block. This rule prevents the failure mode where an array buffer is repeatedly reallocated in a small `longlived` pool, leaving stranded old buffers in the freelist until program end.

### 8.5 Longlived pool layout, headers, and freelists

The `longlived` pool stores all refcounted blocks: array headers, array element buffers, record/union/module bodies, and `__string_block` backing for refcount-managed strings. There is one allocator and one freelist scheme; strings are not a separate allocation universe.

#### Block header

Every longlived block has a fixed 4-byte header:

```c
typedef struct __longlived_header {
  uint16_t size;       // size of payload in bytes (not including this header)
  uint16_t refcount;   // 0xFFFF = static (used for string literals); else live count
  // payload follows
} __longlived_header;
```

The header is at the block's base address. The payload follows immediately. There is no footer.

For strings, the header *is* `__string_block` (§7.1) — same layout. For arrays, the header precedes the array's element buffer slot; the array's `__internal_dynamic_array_t` itself is a separate longlived block whose payload is the array's metadata. For records and unions, the payload is the record/union body.

#### Per-class freelists

Default class set: 8, 16, 32, 64, 128, 256, 512, 1024 bytes (payload size, excluding header). Allocations round up to the next class. Each class has its own freelist (singly-linked through the freed payload).

Class set is tunable in the per-target manifest. After measurement on representative workloads, finer classes may be added at small sizes (e.g. 12, 20, 48) to reduce internal fragmentation.

#### Free-block marker

A block on a freelist has its `refcount` field set to a reserved free-block sentinel (`0xFFFE`). Live blocks have `refcount` in `[1, 0xFFFD]`; static blocks are `0xFFFF`. This lets `reclaim()` (§14.6) identify free blocks during its linear pool scan.

#### `reclaim()` algorithm

```text
walk pool from start to allocator high-water mark:
  read header at current position
  if refcount == 0xFFFE (free):
    look at next physical block (current + sizeof(header) + size)
    if next is also 0xFFFE:
      remove both from their freelists
      merge: header.size += sizeof(next_header) + next.size
      insert merged block into matching size-class freelist (or "miscellaneous" if size doesn't match a standard class)
      advance past merged block
    else:
      advance past current
  else:
    advance past current
```

Linear scan is O(N) where N is the block count. For ZXN's 3 KB longlived pool this is bounded; for the host with multi-MB pools it is run on demand only.

#### Limitation

`reclaim()` mitigates fragmentation only when adjacent free blocks exist. It does not relocate live blocks; a heap fragmented by interleaved live/free blocks cannot be defragmented without relocation, which this ADR rejects (§3, §15.1).

### 8.6 Array operations

| Operation                          | Behaviour                                              | Allocates? |
|------------------------------------|--------------------------------------------------------|------------|
| `arr[i]` (read)                    | Bounds-checked element read                            | No         |
| `arr[i] := v` (write)              | Bounds-checked element write; refcount dec old, inc new if element is handle/descriptor | No |
| `length(arr)`                      | Returns `arr->length`                                  | No         |
| `append(arr, v)`                   | Append; grow if in bump pool, halt if at fixed cap     | Sometimes  |
| `insert(arr, i, v)`                | Insert at index, shift right                           | Sometimes  |
| `pop(arr)`                         | Remove and return last element (out-parameter form internally to avoid copy) | No |
| `remove(arr, i)`                   | Remove at index, shift left, return removed value      | No         |
| `compact(arr)`                     | Shrink element buffer to current `length`              | Recycles   |
| `for x in arr { ... }`             | Iterate; no refcount touch                             | No         |

The earlier `get(arr, i)` and `set(arr, i, v)` builtins are removed (see §13). `arr[i]` and `arr[i] := v` are the canonical forms.

A new `remove(arr, i)` builtin closes the asymmetry with `insert`. (`pop(arr)` removes the last element; `remove(arr, i)` removes at an arbitrary index.)

### 8.7 String arrays

Under the descriptor-only model, `string[]` is an array of `string` descriptors. There is no special case. `append`, `insert`, `set`, `get`, and `pop` copy descriptors only; backing bytes are not duplicated. The `is_string_array` flag is removed (§13).

## 9. Records, unions, modules

Records, unions, and modules are handles to allocated bodies. The body is the payload of a universal block header (§8.5); the refcount lives in that header, not in any aggregate-specific struct. There is no separate `__aggregate_header` type — the universal `__longlived_header` is used for all refcount-managed allocations across the runtime.

Given a record/union/module handle `h`, the header is at `((__longlived_header *)h) - 1`. Refcount semantics are identical to arrays (§8.2 and §8.3, including the bump-pool refcount rule and the address-range check on release).

### 9.1 Pool selection

Same rule as arrays: aggregates whose handle does not escape live in `bump`; aggregates whose handle escapes live in `longlived`.

### 9.2 Constructors

There are two distinct construction forms with different allocation rules:

**Inline literal construction** (`{ field := ..., field := ... }` and `Shape::Variant(args)`): no function-call boundary. Subject to destination-driven allocation (§10.4): the literal allocates directly in the destination's pool when the destination is known.

```rock
team.captain := { name := "x", hp := 100 };
// literal allocates directly in team's region (longlived if team is longlived)
```

**Function-call constructors** (any user-defined `sub` returning a record/union/module): function-call boundary. Materialises in `longlived` via the standard return ABI (§10.3); caller transfers the producer.

```rock
team.captain := makePlayer();
// makePlayer allocates the new player in longlived; caller transfers the
// producer to team.captain
```

There is no hidden region parameter on either form. Inline literals use the lexical destination; function calls use longlived.

### 9.3 Modules

Modules are program-lifetime singletons. They live in `longlived` (or in static storage if their bodies contain no descriptors and no handles). Refcount is initialised to 1 at program start and is never decremented by user code; modules are reclaimed only at program exit.

### 9.4 Typechecker enforcement of structural acyclicity

The typechecker rejects any user-defined type whose field graph is recursive, **including recursion via array, record, union, or string field indirection**. The detection is a standard DFS over the user-type graph:

```
For each user-defined type T:
  walk(T, visited = { T }):
    for each field f of T:
      let f_type = base type of f (unwrapping arrays, descriptors)
      if f_type is in visited: REJECT (cycle through f)
      if f_type is a user-defined type: walk(f_type, visited ∪ { f_type })
```

This rejects types like:

```rock
record Node { Node[] children }     // rejected: Node reaches Node through children
record Tree { Tree left; Tree right }  // rejected: direct recursion (also infinite size)
union Expr { num: int; sum: Expr[] }   // rejected: Expr reaches Expr through sum
```

Diagnostic shape:

```
error: recursive type definition forbidden
  --> game.rkr:14:1
   |
14 | record Node { string name; Node[] children; }
   |                            ~~~~~~~~~~~~~~~ field 'children' contains 'Node'
   |
   note: Rock requires structurally acyclic types so refcount-based
         reclamation is complete. Express tree- or graph-shaped data as
         a flat collection with index references:
           record Node { string name; int parent_idx; }
           Node[N] tree;
```

#### Why this is a deliberate language limitation

Refcount cannot reclaim cycles without auxiliary cycle detection (mark-sweep or trial deletion), which this ADR rejects on cost grounds. Rather than ship cycles silently leak, the language disallows the constructs that could produce cycles. The cost is real and visible: tree- and graph-shaped data must use flat collections with index references rather than recursive type definitions. This is documented as a language-level constraint in §15, not as an implementation detail.

A future ADR may introduce weak references to permit recursive type definitions with one-way ownership; until then the constraint stands.

## 10. Allocation regions and the lifetime partial order

Each value, when allocated, is placed into a specific region. Regions form a **partial order** under the lifetime relation. The order is the scope tree of the program with two virtual roots:

- `static` (top) — string literals, compiler-emitted constants. Lives forever.
- `longlived` (just below static) — refcount-managed allocations in the `longlived` pool. Lives until refcount drops to zero, which is bounded only by program exit for module-rooted state.
- The remaining nodes are the syntactic scope tree: function-body scopes, then nested block scopes, then statement scopes (§5.3).

For two scopes A and B:

- `A ≥ B` (A outlives B) iff A is `static`, A is `longlived`, or A is an ancestor of B in the scope tree.
- A and B are **incomparable** iff neither is an ancestor of the other (sibling `if`/`else` arms; iteration N vs. iteration N+1 of the same loop; two top-level functions).

Incomparable scopes are mutually unreachable at any given program point: if execution is in scope X, it is not simultaneously in any sibling of X. Therefore no store can have a source value from an incomparable scope; the compiler never has to compare them. The partial-order relation is sufficient.

The compiler computes each value's source region from its allocator (literal, constructor call, `concat`, `toString`, `clone`, `read_file`, parameter, return value), and each store's destination region from the slot being written.

### 10.1 The store-region rule

> A store `dest := src` is well-formed iff `region(src) ≥ region(dest)` in the lifetime partial order. When `region(src)` is incomparable to `region(dest)` the store is unreachable and need not be considered. When `region(src) < region(dest)` (source is a strict descendant of destination in the scope tree), the compiler emits an implicit promotion of `src` into `dest`'s region.

Promotion is type-shape-directed:

- **String descriptor**: clone `length` bytes into the destination region; rewrite the descriptor's `data` to point at the new bytes; set the new descriptor's `capacity = length`.
- **Array handle**: allocate a new header and element buffer in the destination region; copy elements (which themselves may be descriptors or handles whose backing already lives in a region of equal or longer lifetime, so no recursive promotion is needed unless a contained handle's backing is shorter-lived, in which case recurse).
- **Record / union / module handle**: allocate a new header and body in the destination region; copy fields; recurse on any field whose backing is shorter-lived.

### 10.2 Implicit promotion is the default

The earlier draft considered making the store-region rule a compile error, requiring the user to write `clone(...)` at the source site. That position is rejected here on ergonomics grounds: the allocation happens regardless, and forcing the user to type it adds friction without information. Implicit promotion is the default.

The cost remains visible to the user through tooling, not source noise (§14).

### 10.3 Function return ABI — uniformly into longlived

There is no hidden region parameter on function calls. The callee allocates non-scalar returns in `longlived` and returns an owned producer (§7.6). Caller-side destination region does not affect the call ABI.

| Declared return type                                              | Callee return action                  | Caller capture |
|-------------------------------------------------------------------|---------------------------------------|----------------|
| `string`, `T[]`, `record R`, `union U`                             | `__return_T(retval)` before unwind; result is owned producer | transfer (no inc) |
| Scalar (`int`, `byte`, `word`, `dword`, `bool`, `char`, `float`)   | return value in register; no allocator action | direct copy    |

The return helper's three internal paths (retain-and-return for longlived, no-op for static, allocate-copy for bump) are runtime decisions inside `__return_T`; the ABI sees a uniform "callee returns an owned producer in longlived semantics."

### 10.4 Destination-driven allocation — literal expressions only

Function calls always materialise non-scalar returns in `longlived` (§10.3); destination-driven allocation does not apply across function-call boundaries because the single LIFO bump pointer cannot accommodate callee-side allocations into a caller scope.

Destination-driven allocation does still apply to **inline literal allocator expressions** that have no function-call boundary:

- Record literals: `{ x := 1, y := 2 }`
- Union construction: `Shape::Circle(r)`
- Array literals: `[a, b, c]`

For these, when the destination's region is known at the allocation site, the compiler allocates directly into that region rather than allocating in the current scope and promoting.

```rock
record Point p := { x := 1, y := 2 };
// p lives in the current block; literal allocates directly in current block's pool

team.captain := { name := "x", hp := 100 };
// team.captain is in team's region; literal allocates directly in team's region
//   (longlived if team is longlived; bump if team is in current bump scope)

int[] xs := [1, 2, 3];
// xs in current scope; literal allocates in current scope's pool
```

Statement scratch (§5.3) is reserved for **non-escaping intermediates** — sub-expressions whose value has no destination beyond the statement.

Function call results land in `longlived` regardless of destination; subsequent capture into a shorter-lived destination is a producer-transfer (§10.6). Hot loops that produce many short-lived function results pay longlived alloc/free per iteration; structural alternatives (lifting the call out of the loop, using inline literals, restructuring) are the user's lever.

### 10.5 Where promotion fires

Promotion is the fallback for stores whose source allocation region was not known at the source site (so destination-driven allocation could not apply). The compiler emits a store-promotion call when source < destination in the lifetime partial order. Function returns are not store promotion; every non-scalar return calls `__return_T` (§10.3).

| Context                                                       | Promotes? |
|---------------------------------------------------------------|-----------|
| Local variable in current block, RHS in current block         | No        |
| Local variable in outer block, RHS in inner block             | Yes (if RHS not destination-allocated) |
| Aggregate field, RHS lifetime shorter than aggregate's region | Yes       |
| Array slot, RHS lifetime shorter than array's region          | Yes       |
| Global / module field, RHS in any block-region                | Yes       |
| Function return (non-scalar)                                  | Calls `__return_T`; materialises owned result in `longlived` (§7.6) |
| Function return (scalar)                                      | n/a       |

The unconditional return materialisation is a soundness requirement (§7.6). Longlived returns retain and return the same backing; bump-backed returns allocate-copy into `longlived`; static returns are unchanged.

### 10.6 Producer / borrower / transfer convention

Every refcounted expression is classified as either a **producer** or a **borrower** by the generator. The classification determines what retain/release the assignment site emits.

#### Producers (rc = 1, owned by the expression)

- Calls to allocating builtins: `concat`, `toString`, `clone`, `read_file`.
- Constructor calls (record / union / module construction).
- Array literal expressions `[a, b, c]`.
- Function calls returning a non-scalar type. (The callee has materialised an owned longlived result; the caller receives a producer.)

#### Borrowers (just naming an existing slot)

- Variable references (`x`, `team`).
- Field reads (`r.field`, `team.captain`).
- Slot reads (`arr[i]`, `state.players[i].score` for non-scalar component).

#### Transfer rule

When the source is a **producer** and the destination is a stable slot (variable initialiser, slot write, return statement), the assignment is a **transfer**:

- No inc on destination side.
- No dec on source side.
- The producer's existing rc = 1 becomes the destination's owned reference.

When the source is a **borrower**, the assignment is a **retain-and-release**:

- Inc on RHS (the borrower).
- Dec on LHS (the slot's previous content).
- Both source and destination now hold strong references.

When a refcounted producer expression appears as the whole of an expression statement (its value is discarded), the generator emits a release at the statement boundary.

#### Worked examples

```rock
string s := concat(a, b);              // producer; transfer to s; s.rc = 1
s := concat(c, d);                      // producer; release old s (rc 1→0, freed); transfer; s.rc = 1
t := s;                                 // borrower; retain s (rc 1→2); release old t (if any); both hold; rc = 2
print(concat(a, b));                    // producer used as expression value; statement-end release; freed
append(arr, concat(a, b));              // producer transferred into slot; arr holds; rc = 1
append(arr, s);                          // borrower retained into slot; rc = 2 (s + arr both hold)
record_field := makePlayer();           // producer transferred into field; field holds; rc = 1
return concat(a, b);                    // producer; materialised by __return_T; transfer to caller's destination
```

This convention is consistent with ARC-style runtimes (Swift, ObjC ARC) and standard refcount-language practice. It eliminates one inc/dec pair on every fresh-allocation assignment, which is the single most common pattern in straight-line code.

## 11. Generator: escape and lifetime analysis

The generator's existing AST walk gains a region-tracking pass that runs immediately after parsing.

### 11.1 Inputs

- The fully-parsed program AST.
- The block scope tree (already implicit in the AST).
- Type information per expression (already computed by `infer_expr_type`).

### 11.2 Outputs (annotations attached to AST nodes)

- Each value-producing expression: `source_region` (a scope-tree node, or `static`, or `longlived`).
- Each store site: `destination_region`.
- Each store site: `needs_promotion` (boolean) and, if true, the type-shape walker to call (which is the generated `__promote_T` for a specific type T — see §11.3).
- Each return statement with non-scalar type: which generated `__return_T` helper to call before unwind.
- Each allocating call (`concat`, `toString`, `clone`, `read_file`, constructors): which region to allocate into. By default, statement scratch; overridden to a longer region by destination-driven allocation (§10.4) when the destination is known.

### 11.3 Generated per-type walker functions

For every Rock type T that appears in the program, the generator emits the following helpers in `<output>.c`:

- `__retain_T(handle_T)` — increments T's refcount.
- `__release_T(handle_T)` — decrements; on zero, walks each field of T calling the appropriate child release (`__string_release` for string fields, `__release_U` for record/union/module-typed fields, `__release_array_X` for array-typed fields), then frees T's body to the size-class freelist.
- `__promote_T(handle_T, region)` — allocates a new T body in `region`, copies fields, recurses with type-shape-driven promotion on any field whose source backing is in a shorter-lived region.
- `__return_T(handle_T)` — materialises T as an owned function result in `longlived`; retain-and-returns longlived non-static sources, returns static unchanged where applicable, and allocate-copies bump sources.
- `__release_array_T(arr_handle)` — decrements the array's refcount; on zero, walks each slot of type T calling `__release_T` (or `__string_release` if T is `string`), then frees the element buffer and header to the freelist.
- `__promote_array_T(arr_handle, region)` — allocates a new array in `region`, copies elements with per-element T promotion.
- `__return_array_T(arr_handle)` — materialises an array as an owned function result in `longlived` using the same return-ownership rule as `__return_T`.

For built-in scalar types these helpers degenerate to no-op walkers and are inlined. For `string`, the helpers are the `__string_retain` / `__string_release` / `__string_promote` / `__return_string` runtime functions described in §7.6.

This per-type generation is what makes recursive cleanup of nested aggregates correct from `elem_size` alone: the generator emits a walker that knows T's exact shape; runtime release calls the right walker.

### 11.4 Decisions made by the pass

- **Pool placement of allocations**: bump if the value's effective lifetime fits within its allocating scope; longlived otherwise. "Effective lifetime" is the deepest scope containing all uses of the value (dominance computation over the scope tree).
- **Allocation region selection**:
  - For inline literal expressions (§10.4): by default the allocating scope; overridden to the destination's region when the destination is known at the allocation site.
  - For function calls returning non-scalar values: always `longlived`. The compiler does not pass any region to the callee; the callee uses `__return_T` (§11.3) to materialise the result in `longlived`.
- **Retain/release emission**: at every site listed in §7.6 (strings) and §8.2 (arrays/records/unions). The pass marks each AST location.
- **Promotion emission**: at every store site where `needs_promotion` was set.
- **Return materialisation emission**: per §10.3, every return statement whose expression type is `string` / `T[]` / `record R` / `union U` calls the generated `__return_T` helper before unwind.

### 11.5 Soundness

The pass is sound because:

- The lifetime relation is a partial order on the syntax tree (§10). For any two values whose lifetimes need to be compared, both are in scope at a common program point, so they are ordered.
- The structural-acyclicity invariant (§9.4) is established by the typechecker before this pass runs, so the recursive promotion and release walks terminate.
- Callee-side parameter retain on entry / release on exit (§7.6) is sound under the no-threading + no-stored-callback assumptions (§2): the parameter's refcount is stable for the call duration, so the callee can overwrite caller-side slots (including the only slot that originally held the parameter's value) without invalidating its own borrowed parameters.
- Returns are sound because `__return_T` materialises every non-scalar return as an owned producer in `longlived` (§7.6, §10.3), independent of any caller-side region.

### 11.6 Completeness

Every store falls into one of: same-region (no action), source ≥ destination but type-shape requires no copy (no action), source < destination (promotion required), or unreachable (sibling-scope cross — never observed). The pass therefore makes a decision for every store. There is no fall-through default.

### 11.7 Early exits and unwinding

`return`, `halt`, and any future `break` / `continue` must restore the bump pointer to the appropriate save point AND release every owned long-lived local in the scopes being unwound. The earlier `emit_return_cleanup` helper is removed (§13); it is replaced by a unified `emit_unwind_to(target_scope)` that handles all early-exit forms.

#### Per-scope cleanup record (compile-time generator structure)

The generator maintains a stack of per-scope cleanup records during codegen. Each record is a compile-time structure used to produce the unwind sequence; it is not a runtime data structure.

```
struct cleanup_record {                  // compile-time only
  bump_save_t        bump_mark;           // generator-side identifier for the save point
  ast_t              scope_node;          // AST node defining this scope
  owned_local_t      locals[];            // every refcounted local owned by this scope
};

struct owned_local_t {
  string_view        name;                // local's identifier in generated C
  type_descriptor_t *type;                // the local's Rock type — drives release helper selection
};
```

`type_descriptor_t` carries enough information for the generator to emit the correct per-type release call:

- For string locals: emit `__string_release(name)`.
- For array locals of element type T: emit `__release_array_T(name)`.
- For record/union/module locals of type T: emit `__release_T(name)`.
- For scalar locals: no emission (scalars have no refcount).

Entries are pushed on scope entry and popped on normal scope exit (in which case cleanup runs as part of the pop). On early exit, the generator walks entries from the innermost active scope outward.

#### `emit_unwind_to(target_scope)`

```text
for each cleanup_record c in inner-to-outer order, until c.scope_node == target_scope:
  for each local in c.locals:
    emit the type-specific release call selected by local.type:
      string                    -> __string_release(local.name)
      string[] / T[]            -> __release_array_T(local.name)
      record R / union U / module M  -> __release_R(local.name)  (etc.)
      scalar                    -> no emission
  emit bump pointer restore to c.bump_mark
```

This emits the full unwinding sequence inline at every early-exit site. No exception tables, no runtime unwinder — straight-line generated code dispatched per type at compile time.

#### Form-specific application

| Early-exit form  | Target scope                                        | Additional emission                                |
|------------------|-----------------------------------------------------|----------------------------------------------------|
| `return expr`    | The function body                                    | First evaluate `expr`, run callee return materialisation (§7.6), unwind, emit `return` |
| `return` (void)  | The function body                                    | Unwind, emit `return`                              |
| `halt(code)`     | (program exit)                                       | Skip per-scope unwinding (pools deinitialise wholesale); call `__pool_deinit_all`; exit |
| `break` (future) | The enclosing loop's body scope (one level out)      | Unwind to loop-body scope; jump to loop exit label |
| `continue` (future) | The enclosing loop's body scope                  | Unwind to loop-body scope; jump to loop continuation |

#### Interaction with return materialisation

The return value is computed and materialised **before** the unwind walks the function-body's locals. This matters: if the return expression references a longlived local (e.g. `return team.captain`), the local's refcount must still be live during the return helper's retain/copy check. Order:

1. Evaluate `return expr` into a temporary.
2. Call `__return_string(temp)` (or type-shape equivalent) — this retain-and-returns longlived non-static sources, returns static unchanged, or clone-copies bump sources into `longlived`.
3. Run `emit_unwind_to(function_body)` — release all owned locals (including any temporaries from step 1 that are no longer needed; the returned value's reference is held by the owned result from step 2).
4. Emit the C `return` with the promoted value.

### 11.8 Loop-carried lifetime escape rule

Destination-driven allocation (§10.4) places an inline literal value into the destination's region. For a slot like `team` declared *outside* a loop, with `team := { name := "x" }` (a literal, not a function call) written *inside* the loop, naive destination-driven allocation would put each iteration's team into the outer block's bump region. Bump regions reclaim by pointer-restore at scope exit, so each iteration's old team would remain bumped in the outer region until the outer block exits. The bump pool grows linearly in the iteration count.

(Function-call writes like `team := makeTeam()` are unaffected; function returns always materialise in `longlived` per §10.3, so iteration-old values are released and freelisted on the overwrite. The loop-carried rule exists specifically for the literal/destination-driven case.)

This is unsound for any loop that runs many times within an enclosing block.

#### The rule

> A composite-typed slot whose declaring scope is an ancestor of any loop body scope, and which is written from inside that loop body, is allocated in `longlived` rather than in its declaring scope's bump region.

Loop-carried writes are detected by the escape-and-lifetime analysis pass:

```text
for each composite slot S declared in scope D:
  for each write to S in the program:
    let W = the scope containing the write
    if W is a (transitive) descendant of D AND W is inside a loop body B
       AND B is a (transitive) descendant of D:
      mark S as longlived
      break
```

The slot's *handle storage* (the variable holding the pointer) can stay in any scope; the *values it holds* are allocated in longlived. Overwrite triggers normal refcount-drop-and-freelist-free, so the iteration N value is reclaimed when iteration N+1 overwrites it.

#### What is and isn't loop-carried

Loop-carried (rule applies — inline literal write):

```rock
{
  record Team team := { name := "alpha", hp := 100 };   // declared in outer block
  for i in 1 to 100 {
    team := { name := "beta", hp := 100 };               // literal write from inside loop body
  }
}
```

Not loop-carried (function-call write — already in longlived per §10.3):

```rock
{
  record Team team := makeTeam();
  for i in 1 to 100 {
    team := makeTeam();              // function call; materialises in longlived; old team released and freed on overwrite
  }
}
```

Not loop-carried (rule does not apply; iteration-local):

```rock
for i in 1 to 100 {
  record Team team := { name := "x", hp := 100 };   // declared inside loop body; bump-allocated; iteration reset reclaims
}
```

Not loop-carried (read-only):

```rock
{
  record Team team := { name := "x", hp := 100 };
  for i in 1 to 100 {
    print(team.name);                 // read only; team stays in outer bump region
  }
}
```

#### Cost

The rule moves loop-carried slots from bump to longlived. Each overwrite then incurs a freelist alloc + free pair instead of a bump pointer increment. The trade is correct memory accounting (no growth) for slightly higher per-iteration cost. For the alternative (bump growth proportional to iteration count) the program would simply OOM.

This rule is part of the escape-and-lifetime analysis pass (§11.4); it is detected and applied without source-level annotation.

## 12. User-visible API

The user-facing surface for memory management is small.

### 12.1 Builtins

| Builtin                     | Purpose                                                                 |
|-----------------------------|-------------------------------------------------------------------------|
| `clone(s)`                  | Allocate an independent copy of a string. Use for alias-isolation.      |
| `clone(arr)`                | Allocate an independent deep copy of an array.                          |
| `compact(arr)`              | Shrink an array's element buffer to `length`. Returns the array; old buffer returned to its size-class freelist. |
| `reclaim()`                 | Force a freelist coalescing pass over the `longlived` pool. Walks address-ordered free blocks; merges physically adjacent free blocks into the next larger size class. Does not move live data; only mitigates fragmentation when adjacent free blocks exist. |
| `length(s)` / `length(arr)` | Existing builtin; no change                                             |
| `set_string_index_base(b)`  | Existing builtin; preserved. Default index base for string operations remains 1 by convention. |

### 12.2 No new keywords

No `region`, no `arena`, no `scope`, no `borrow`, no `move`. Memory management is fully implicit at the syntax level. The block scope already in use for control flow doubles as the lifetime scope.

### 12.3 Removed builtins

| Removed                | Replacement                                                |
|------------------------|------------------------------------------------------------|
| `get(arr, i)`          | `arr[i]`                                                   |
| `set(arr, i, v)`       | `arr[i] := v`                                              |
| `new_string(s)`        | `clone(s)`                                                 |
| `string_to_cstr(s)`    | Internal-only helper renamed `__string_clone_cstring(s, region)` for the host-only file-path case. Not user-visible. |

## 13. Removed runtime / generator machinery

The following must be removed in the same change. After the change, `rg "owned|__free_string|TRACK_STRING|is_string_array|track_string_var|track_string_tmp|emit_return_cleanup" src` returns no live source matches.

- `owned` field on `struct string`
- `__free_string()` function and its callers
- `TRACK_STRING` enum variant in `track_kind_t`
- `is_string_array` flag on `tracked_var_t` and in `__internal_free_array()` signature
- `track_string_var()`, `track_string_tmp()` helpers
- `emit_return_cleanup()` helper (return cleanup folds into the unified scope-cleanup path with no string-specific case)
- The string-specific deep-copy paths in record-construction codegen
- The `_Generic`-dispatched `to_int`/`to_byte` integer-literal cast pre-existing noise paths in tests are already cleaned up; this ADR does not require additional test changes for casts beyond what TODO.md tracks separately.

The currently-open TODO item *"Type parameter info for the rest of the builtins"* in `TODO.md` is closed as part of this work, not after. All builtins use `register_builtin_typed` with full parameter type info; the half-and-half state ends.

## 14. Diagnostics and tooling

### 14.1 `--explain-allocations`

A compiler flag that, when set, dumps every promotion the generator emitted, with file:line and promotion kind. Example output:

```
game.rkr:42:18  promote(STRING_CLONE)  upper(toString(n))  -> team.log array region
game.rkr:55:5   promote(ARRAY_DEEP)    spawnEnemies()       -> level.enemies field region
```

This is the user's window into "where does my program allocate?" without polluting source.

### 14.2 Debug-build region accounting

In debug builds, every region maintains a high-watermark counter and a per-block allocation count. At block exit the runtime asserts that the bump pointer returns to its saved value (catches generator bugs). At program exit the runtime asserts that the longlived pool's outstanding refcount is zero except for module singletons (catches leaks).

### 14.3 Debug-build alloc-site tagging

In debug builds, each `longlived` allocation header carries a 4-byte `alloc_site` (file/line packed). The leak detector at `kill_compiler_stack` prints alloc sites of any non-zero-refcount headers. Double-free detection halts with the alloc site of the original allocation.

### 14.4 Fragmentation watchdog (debug builds)

A periodic check (e.g. on each `longlived` allocation) computes `largest_free_block / total_free`. If the ratio drops below 0.5, the runtime emits a warning. This makes fragmentation visible during development rather than as a mysterious OOM in the field.

### 14.5 No release-build overhead

All of §14 is debug-only. Release builds pay zero cycles for diagnostics.

## 15. Non-goals and deliberate language limitations

### 15.1 Non-goals (out of scope for this ADR)

- Garbage collector (mark-sweep, tracing, generational, or otherwise).
- Cycle detector. Cycles are forbidden by the typechecker (§9.4); no runtime cycle handling is required.
- Relocating compaction. Fragmentation is bounded by size-class discipline; the way out of fragmentation pressure is `compact(arr)`, `reclaim()`, and pre-sized fixed-capacity arrays.
- Implicit `clone` insertion at user-visible source sites for "safety". Promotion happens at the generator level only; user-visible `clone` is a deliberate operation.
- A second string type (`stringbuf`, `mut_string`, etc.). The single `string` type with `capacity` and `backing` fields covers both view and writable cases.
- Per-program override of allocator algorithm. Pools are bump or freelist; the choice is fixed per pool.
- Backwards-compatibility flag for the old memory model. The change is atomic.
- Lifetime annotations on function signatures. Borrow semantics + scope-tree analysis cover what they would.
- Weak references. Possible future feature to enable recursive type definitions; a separate ADR if and when needed.

### 15.2 Deliberate language limitations

These are restrictions on what Rock programs can express, accepted as part of the memory model's cost/benefit choice. They are limitations of the **language**, not omissions of this ADR.

- **Recursive type definitions are forbidden** (§9.4). `record Node { Node[] children }` and similar types are rejected at compile time. Tree- and graph-shaped data must use flat collections with index references. Cost: less ergonomic linked-data structures; benefit: refcount-based reclamation is complete with no cycle collector.
- **Long-lived dynamic arrays must declare a fixed capacity.** `int[] xs := []` with a long-lived destination is rejected. `int[N] xs` is the long-lived form. Cost: capacity must be planned; benefit: no stranded buffers from realloc-style growth in `longlived`.
- **In-place string growth is not supported.** `setCharAt` cannot extend `length` past `capacity`. To produce a longer string, use `concat`. Cost: extra allocation for length-changing operations; benefit: simple capacity model with no realloc complexity.
- **`string` storage is not directly mutable from C interop without `__string_clone_cstring`.** Arbitrary views are not null-terminated, so passing `s.data` directly to a C API expecting a null-terminated buffer is unsound; the helper allocates a terminated copy in the destination region.

## 16. Test plan

The change is gated behind passing the test plan in full. Tests are added in the same change set, not after.

### 16.1 Existing test suite — migrated, not unchanged

All 395 existing tests must pass after migration to the post-change syntax surface. Specifically:

- Calls of the form `get(arr, i)` are rewritten to `arr[i]` in the same change set.
- Calls of the form `set(arr, i, v)` are rewritten to `arr[i] := v`.
- Any `new_string(s)` is rewritten to `clone(s)`.
- Any uncapacitied-array declarations whose handle escapes their declaring block are rewritten to declare a capacity.

Test migration is part of this change, not a follow-up. The behavioural floor is: zero functional regressions across the migrated suite.

### 16.2 New behavioural tests

Added under `test/` with the existing `Assert.rkr` harness.

| Test file                              | Covers                                                                |
|----------------------------------------|-----------------------------------------------------------------------|
| `test/string_view_test.rkr`            | substring is a view; equals/print on non-null-terminated views        |
| `test/string_capacity_test.rkr`        | setCharAt on capacity>0 succeeds; on literal/substring halts cleanly  |
| `test/string_alias_mutation_test.rkr`  | Mutation through alias is observable through other aliases            |
| `test/string_clone_test.rkr`           | clone yields independent backing; mutation through clone does not affect source |
| `test/string_array_shallow_test.rkr`   | string[] append/insert/set/get/pop preserve descriptors without byte-copy |
| `test/array_remove_test.rkr`           | remove(arr, i) shifts left and returns removed element                |
| `test/array_compact_test.rkr`          | compact(arr) preserves contents and recovers freelist memory          |
| `test/array_refcount_test.rkr`         | Overwritten array handle in long-lived slot reclaims old buffer       |
| `test/array_fixed_capacity_test.rkr`   | append beyond fixed capacity halts with diagnostic                    |
| `test/escape_promotion_test.rkr`       | append into long-lived array auto-promotes a function-scratch string  |
| `test/escape_return_substring_test.rkr`| return substring(local_concat, 1) returns owned longlived value via __return_string |
| `test/region_iteration_test.rkr`       | Loop iteration body resets region per iteration; no accumulation      |
| `test/region_nested_block_test.rkr`    | Plain `{ ... }` block resets at brace close                           |
| `test/oom_bump_test.rkr`               | Bump pool exhaustion halts with named diagnostic                      |
| `test/oom_longlived_test.rkr`          | Longlived pool exhaustion halts with named diagnostic                 |
| `test/pool_layout_test.rkr`            | User-reserved range is unmolested by allocations                      |
| `test/string_backing_refcount_test.rkr`| Long-lived string backing is reclaimed when last descriptor drops    |
| `test/string_substring_retain_test.rkr`| Substring view of longlived backing keeps backing alive               |
| `test/record_with_string_release_test.rkr` | Releasing a record releases its string fields' backing            |
| `test/array_of_records_with_strings_test.rkr` | Releasing an array walks elements and their string fields      |
| `test/recursive_type_rejected_test.rkr`| Recursive type definition rejected with diagnostic                    |
| `test/return_borrowed_string_test.rkr` | id(p) where p is a longlived slot — refcount math is correct (caller's slot + p slot both hold; rc=2; no leak, no double-free) |
| `test/return_pass_through_bump_test.rkr` | id(local_bump_string) returns owned longlived copy; original bump source dies with callee; caller's destination remains valid |
| `test/return_longlived_borrower_test.rkr` | Returning a longlived borrower parameter retains a caller-owned result |
| `test/return_fresh_producer_loop_test.rkr` | Returning a freshly allocated producer in a loop survives callee unwind and is reclaimed on caller overwrite/discard |
| `test/destination_driven_alloc_test.rkr` | record literal in declaration allocates directly in block region (no copy); function-call results land in longlived |
| `test/reclaim_coalesce_test.rkr`       | reclaim() coalesces adjacent free blocks; non-adjacent stay separate  |
| `test/host_zxn_budget_test.rkr`        | Test program runs cleanly under host build with ZXN manifest sizes    |
| `test/loop_carried_longlived_test.rkr` | team := makeTeam() in a loop body uses longlived; pool does not grow with iteration count |
| `test/bump_alias_release_test.rkr`     | Aliased bump arrays/records release children exactly once; no double-release, no leak |

### 16.3 Structural tests

Run as part of CI, not as Rock-source tests:

```sh
rg "owned|__free_string|TRACK_STRING|is_string_array|track_string_var|track_string_tmp" src
# expected: no output

rg "register_builtin\b" src/generator.c
# expected: no output (all converted to register_builtin_typed)
```

### 16.4 ZXN smoke tests

The ZXN target builds and runs:

- `test/array_test.rkr`
- `test/byte_test.rkr`
- `test/format_test.rkr`
- `test/string_view_test.rkr` (new)
- `test/region_iteration_test.rkr` (new)
- `test/array_refcount_test.rkr` (new)
- `test/string_backing_refcount_test.rkr` (new)

Each produces a `.nex` file and runs without runtime fault under the default ZXN manifest. SDCC warnings about unused parameters or implicit conversions are absent.

### 16.5 Host with ZXN budget profile

A host build mode runs the host (gcc) target with the ZXN manifest's pool sizes. Invoked as:

```
rock --target=host --memory-profile=zxn  myprog.rkr
```

CI runs the entire test suite (16.1, 16.2) under this profile. Any test that succeeds with the default host pool sizes but exhausts a pool under ZXN sizes fails the CI gate. This catches memory regressions on the host before deployment to device, where on-device debugging is significantly more expensive.

The ZXN smoke tests (16.4) are additionally run on-device after host-budget validation passes, as the final gate.

## 17. Implementation work breakdown

The change is implemented as a single PR series. The phases below describe build order — what depends on what — not separate releases. No phase is mergeable on its own.

### Phase A — Pool architecture and ZXN budget measurement

- Build a representative test program with the existing toolchain. Examine `.map` output from z88dk to derive measured budgets: CRT footprint, code+rodata, RTL state, stack reservation. The default ZXN pool sizes in §4.3 are placeholders to be replaced with measured values before this ADR moves from `draft` to `accepted`.
- Implement `__pool_bump_alloc`, `__pool_bump_save`, `__pool_bump_restore` in `src/lib/alloc.c`.
- Implement `__pool_longlived_alloc`, `__pool_longlived_free` per §8.5: 4-byte block headers `{size, refcount}`; per-class freelists; free-block sentinel (`refcount = 0xFFFE`); static-block sentinel (`refcount = 0xFFFF`).
- Implement `reclaim()` runtime per §8.5: linear walk over the longlived pool from start to high-water mark; identify free blocks via the free-block sentinel; merge physically adjacent free blocks; insert merged blocks into the matching size-class freelist or a "miscellaneous" freelist if size doesn't match a class.
- Add per-target manifest reader; replace `init_compiler_stack()` with `init_pools(manifest)`.
- Update `zpragma_zxn.inc` with the **measured** default ZXN pool layout.
- Add OOM diagnostics keyed on pool name.
- Implement `--memory-profile=zxn` host mode that initialises pools at ZXN sizes.

### Phase B — Typechecker enforcement of acyclicity

- Implement the structural-acyclicity check in `typechecker.c` (DFS over user-type field graph, including walks through array/handle indirection).
- Emit the diagnostic shape from §9.4 when a recursive type is rejected.
- This phase precedes generator changes because subsequent phases assume the acyclic invariant holds.

### Phase C — Block-region and statement-region emission

- Generator emits bump-pointer save on block entry, restore on block exit, for every block form in §5.1.
- Same emission at statement boundaries (§5.3), modulo destination-driven allocation (§10.4).
- Plain `{ ... }` admitted as a freestanding statement form in `parser.c`.
- Remove the existing `scope_t`/`tracked_var_t` linked-list scope tracker (replaced by bump save/restore for non-refcounted values; refcount handles long-lived state).

### Phase D — Descriptor and handle layout changes

- `struct string`: replace `owned` with `{capacity, backing}` per §7.1 in `src/lib/typedefs.h`.
- `__internal_dynamic_array_t`: add `refcount` field.
- Add `__aggregate_header` for records, unions, modules.
- Add `__string_block` header for refcounted string backing in `longlived`.
- Update all generator emissions of allocations to set initial values (`capacity`, `backing`, `refcount`).

### Phase E — Refcount runtime + per-type generated walkers

- Implement `__string_retain`, `__string_release`, `__array_retain`, `__array_release`, `__aggregate_retain`, `__aggregate_release` in `src/lib/fundefs_internal.c`. Static-sentinel and free-sentinel checks per §7.1 / §8.5.
- Generator emits per-type `__retain_T`, `__release_T`, `__release_array_T` for every used type T (§11.3).
- Generator emits retain/release/transfer at every site listed in §7.6 and §8.2 according to the producer/borrower convention (§10.6).
- **Parameter ABI**: callee retains every refcounted parameter on entry; releases on exit (§7.6).
- **Return ABI**: callee invokes `__return_T` before unwinding; caller captures the result as a transfer from a producer (§7.6, §10.5).

### Phase F — Escape and lifetime analysis

- Implement the region-tracking pass described in §11. Annotate the AST.
- Generator consumes annotations to choose pool for each allocation, emit store-promotion calls, emit `__return_T` for non-scalar returns (§10.3), and apply destination-driven allocation (§10.4).
- Implement producer/borrower classification per §10.6.
- Implement the loop-carried lifetime escape rule per §11.8: any composite slot whose declaring scope is an ancestor of a loop body and which is written from inside that body is upgraded to longlived.

### Phase G — Promotion runtime + return materialisation walkers

- Implement `__string_promote(s, region)` runtime helper. Returns the input descriptor unchanged when source is in static or in a region ≥ destination; allocates and copies otherwise.
- Implement `__return_string(s)` runtime helper. Returns static unchanged, retain-and-returns longlived non-static backing, and allocate-copies bump backing into `longlived`.
- Generator emits per-type `__promote_T(handle, region)`, `__promote_array_T(arr, region)`, `__return_T(handle)`, and `__return_array_T(arr)` for every used type T.
- Recursive descent over nested aggregates as required by the type-shape.

### Phase H — Early-exit unwinding

- Implement the per-scope cleanup-record stack in the generator (§11.7).
- Implement `emit_unwind_to(target_scope)` and apply at every `return` and `halt` emission site.
- Reserve the same machinery for future `break` / `continue`.

### Phase I — String semantics

- Make `substring` return a view (capacity = 0; backing inherited).
- Emit static `__string_block` headers for every string literal with `refcount = 0xFFFF` (§7.1).
- Replace `printf("%s", expr.data)` codegen with length-aware emission via `print(expr)` or equivalent.
- Update `write_string_to_file` to use `fwrite(s.data, 1, s.length, f)`.
- Re-spec `setCharAt` against `s.capacity > 0`.

### Phase J — Surface API + test migration

- Remove `get(arr, i)` and `set(arr, i, v)` builtins from the generator.
- Add `remove(arr, i)`, `compact(arr)`, `reclaim()` builtins.
- Add `clone(arr)` and `clone(s)` builtins.
- Reject uncapacitied arrays escaping their declaring block with a clear diagnostic (in escape analysis).
- Migrate all 395 existing tests: `get`/`set` calls rewritten to bracket syntax; `new_string` rewritten to `clone`; uncapacitied long-lived arrays rewritten to fixed-capacity. Behavioural floor: zero functional regressions.

### Phase K — Builtin typing completeness

- Convert every remaining `register_builtin` call to `register_builtin_typed` with full parameter types.
- Closes the `TODO.md` item *"Type parameter info for the rest of the builtins"*.

### Phase L — Removal sweep

- Delete `__free_string`, `TRACK_STRING`, `is_string_array`, `track_string_var`, `track_string_tmp`, `emit_return_cleanup`, and any related dead branches.
- Verify with `rg` (§16.3).

### Phase M — Diagnostics

- Implement `--explain-allocations` flag and its emission path.
- Implement debug-build region accounting, alloc-site tagging, fragmentation watchdog.

### Phase N — Wiki and ADR updates

- Rewrite `wikiroot/pages/concepts/string-representation.md`.
- Rewrite `wikiroot/pages/concepts/array-internals.md`.
- Update `wikiroot/pages/syntax/strings.md`, `wikiroot/pages/syntax/arrays.md`, `wikiroot/pages/syntax/builtins-and-io.md`, `wikiroot/pages/syntax/syntax-index.md`.
- Mark ADR-0002 as superseded by ADR-0003.
- Promote ADR-0003 from `draft` to `accepted` upon merge.

### Phase O — Tests

All tests from §16 added before merge. CI gates on the structural checks in §16.3 plus all behavioural tests passing on both targets and under `--memory-profile=zxn`.

### Estimated scope

| Component                                                   | Approx LoC      |
|-------------------------------------------------------------|-----------------|
| Pool runtime + `reclaim()` + manifest reader                | ~500            |
| Typechecker acyclicity check                                | ~100            |
| Block + statement region emission                           | ~200            |
| Refcount runtime + per-type generated walkers + parameter ABI | ~600         |
| Escape and lifetime analysis + producer/borrower classification | ~750         |
| Promotion runtime + per-type generated promoters / return materialisers | ~350  |
| Early-exit unwinding emission                               | ~150            |
| String / array semantics changes                            | ~300            |
| Builtin typing completion                                   | ~200            |
| Test additions + existing-test migration                    | ~800            |
| Diagnostics and debug machinery (incl. host budget profile) | ~250            |
| Wiki updates                                                | ~500 (markdown) |
| **Total**                                                   | **~4,700 LoC**  |

This is a large but bounded change. There is no incremental shippable subset; every part is needed for the final state to be coherent.

## 18. Resolved open questions from ADR-0002

| Question (ADR-0002 §Open Questions)                                  | Resolution                              |
|-----------------------------------------------------------------------|-----------------------------------------|
| Should `string.data` become `const char *`?                           | No. Mutation is permitted under capacity gating; constness is enforced by the runtime check, not the C type. |
| Should `clone` allocate in program arena only?                        | No. Clone allocates in the destination region selected by escape analysis (caller's result region for return, slot's region for store, current block's bump for local). |
| Should `printf` route Rock strings via `print` or grow a Rock formatter? | Route via `print`. C `printf("%s", ...)` is replaced everywhere for Rock string expressions. A future Rock-native formatter is a separate ADR. |
| Auto-promote vs. explicit clone at source?                            | Auto-promote. Explicit `clone(s)` remains available for deliberate isolation. |
| Should arrays carry their owning region in the header?                | No. The handle's region is determined by the array's pool (bump vs. longlived); the pool is determined at allocation by escape analysis. |
| Should dynamic arrays require explicit initial capacity on small targets? | Yes — but only when the array escapes its declaring block. Local arrays may grow within the bump pool. |

## 19. Open questions remaining

| Question                                                              | Why deferred                              |
|-----------------------------------------------------------------------|-------------------------------------------|
| Optimal default size-class set for the ZXN `longlived` pool           | Awaits measurement after foundation port. The default class set is power-of-two until a measured workload justifies finer classes. |
| Measured ZXN pool sizes (§4.3 placeholders)                           | Established during Phase A from `.map` measurements before the ADR moves from `draft` to `accepted`. |
| Whether to add automatic coalescing on every freelist return          | Skip in v1. `reclaim()` provides on-demand coalescing. Auto-coalescing on every release adds per-call cost; add only if measured workloads show fragmentation pressure between explicit `reclaim()` calls. |
| Whether to expose pool size pragmas at module granularity vs. program granularity | Programs have one set of pragmas applied at the top-level entry module. Module-level overrides are not in scope for this ADR. |
| Future weak-reference language feature for recursive type definitions | Not in scope. A separate ADR if and when a concrete use case demands it. |
| Borrow-safety analysis to elide callee-side parameter retain/release for proven-safe functions | Not in scope. Default ABI is unconditional retain/release for soundness (§7.6). Optimisation requires whole-program effect analysis; consider when measurement shows the parameter inc/dec pair to be a hot-path cost on representative workloads. |

These are the only deferrals. They are tunings of fixed mechanisms, not architectural omissions.

## 20. Consequences

### Positive

- One ownership rule across all types — strings, arrays, records, unions, modules. No per-type special cases.
- Memory layout is fixed and predictable. Inline asm, sprite buffers, raw data, screen memory have stable addresses guaranteed by the user-reserved range.
- **Compiler-managed allocations are provably leak-free under three conditions**: (1) the typechecker's structural-acyclicity rule (§9.4) is enforced; (2) the generator emits retain/release at every site listed in §7.6 and §8.2 (verified by the structural CI checks in §16.3 and the debug-build refcount-zero leak detector in §14.2); (3) the program does not use `@embed asm` or pool override pragmas to construct unmanaged references. Bump-pool allocations are reclaimed at scope exit by save/restore. Longlived-pool allocations are reclaimed at refcount-zero by the per-type generated release walkers. There is no third reclamation path; if a program leaks under these conditions, it is a generator or runtime bug.
- `compact(arr)` and `reclaim()` give application-level control over fragmentation.
- The user writes ordinary code; the compiler inserts the necessary retains, releases, store promotions, and return materialisation.
- OOM diagnostics name the exhausted pool, the requested size, and the available size — actionable rather than mysterious.
- The `--memory-profile=zxn` host mode catches budget overflows on the host before deployment, where debugging is significantly cheaper than on-device.

### Negative

- Refcount adds ~25–40 cycles per array / aggregate / longlived-string assignment on Z80, plus 2 bytes per array header, 2 bytes per aggregate header, 2 bytes per longlived `__string_block`, 2 bytes per string descriptor (the new `backing` field), and 2 bytes per longlived block for the `size` field in the header.
- **Function call overhead**: callee-side retain on every refcounted parameter on entry, release on exit. ~50 cycles per parameter per call. For a function with three composite parameters this is ~300 cycles per call. A future borrow-safety analysis (§19) could elide this for proven-safe functions.
- **Return overhead**: callee invokes `__return_T` on every non-scalar return; the result always materialises in `longlived`. Static returns are unchanged; longlived returns retain once; bump-backed returns allocate-copy into `longlived`. Hot loops that produce many short-lived function results will hit longlived alloc/free per iteration. Mitigation: use inline literals where possible; lift the call out of the loop.
- **Inline literal vs. function-call asymmetry**: `record p := { ... }` may allocate in the destination's bump region (cheap), while `record p := makeP(...)` always allocates in longlived (more expensive but sound). This is visible in the cost profile but not in semantics.
- Long-lived dynamic arrays must declare a capacity. Programs that previously used `int[] xs := []` for long-lived storage must change.
- Long-lived heap fragmentation is bounded but not zero. Pathological mixed-size traffic can pressure the freelist; `reclaim()` mitigates only when adjacent free blocks exist and does not guarantee recovery from arbitrarily fragmented states.
- The atomic-completion constraint means a large PR series (~4,700 LoC plus tests and wiki) with high reviewer burden.
- Recursive type definitions are rejected at compile time. Programs that want tree- or graph-shaped data must restructure to flat collections with index references (§9.4, §15.2).
- String literals carry a 2-byte sentinel header in static rodata. ~100 bytes for a typical 50-literal program.

### Neutral

- The 32 KB target requires up-front capacity budgeting, which is expected on such systems regardless of memory model.
- Programs that hit `longlived` OOM have application-level recovery options (`compact`, `reclaim`, smaller capacities, structural changes) but no runtime-level fallback.
- The leak-freedom guarantee is conditional, not absolute. The conditions are checkable: the structural CI grep ensures the generator emits the correct calls, the typechecker enforces acyclicity, and `@embed asm` is the user's responsibility. Programs that meet these conditions have no silent leakage.

## See Also

- [[decisions/ADR-0001-function-overloading-arity-only]] — superseded scope: arity-only overloading remains current.
- [[decisions/ADR-0002-string-view-memory-model]] — superseded by this ADR.
- [[decisions/ADR-0003-implementation-plan]] — mutable execution checklist for implementing this ADR.
- [[concepts/string-representation]] — must be rewritten as part of Phase L.
- [[concepts/array-internals]] — must be rewritten as part of Phase L.
- [[syntax/strings]], [[syntax/arrays]], [[syntax/builtins-and-io]] — surface-level updates in Phase L.
- [[targets/zxn-z80]] — pool layout is target-specific configuration described here.
