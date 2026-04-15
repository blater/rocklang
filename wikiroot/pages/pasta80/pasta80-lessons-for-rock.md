---
title: "PASTA/80 Lessons for Rock's RTL"
category: pasta80
tags: [pasta80, rock, rtl, strategy, lessons, architecture]
sources: [pasta80]
updated: 2026-04-12
status: current
---

> **Note (2026-04-12):** Partially applied. The keyboard pilot under [[rtl/rtl-overview]] realises lessons 1–4 (thin HAL, mixed C/ASM split, host stubs, flat builtins). Lessons 5–8 remain as rules for future components.

# PASTA/80 Lessons for Rock's RTL

Analysis of PASTA/80's runtime library with concrete recommendations for Rock's cross-platform RTL strategy. Organised as: **pattern observed** → **lesson for Rock** → **recommendation**.

---

## Patterns to Adopt

### 1. The Block* HAL Pattern

**Observed:** PASTA/80's `files.pas` is a ~620-line shared implementation of buffered text and typed file I/O. It works on CP/M, Next, and Agon without modification because all three platforms implement the same 12 `Block*` procedure signatures with platform-specific internals.

**Lesson:** A thin, well-defined HAL layer is the single most effective way to maximise code sharing across targets. The key is choosing the right abstraction level — PASTA/80's "128-byte record" abstraction maps directly to CP/M's BDOS semantics and is simple enough for esxDOS and MOS to implement.

**Recommendation for Rock:**
- Define a C-level HAL with a small set of function pointers or `#ifdef`-switched implementations:
  - `rock_file_open`, `rock_file_close`, `rock_file_read`, `rock_file_write`, `rock_file_seek`
  - `rock_console_putc`, `rock_console_getc`, `rock_console_keypressed`
  - `rock_mem_alloc`, `rock_mem_free` (if diverging from standard malloc)
- Keep the HAL surface small (~15 functions). All higher-level Rock RTL code (string formatting, array operations, I/O formatting) should be written against the HAL, not against platform APIs directly.
- Choose an abstraction level appropriate to Rock's targets: byte-oriented I/O (not 128-byte records) may be more natural for Rock's C backend.

### 2. Layered Composition via Include

**Observed:** Each target has a thin entry-point file (`next.pas`, `agon.pas`, `cpm.pas`) that `{$i}`-includes shared layers. The dependency graph is simple and explicit — no build system magic, no dynamic linking.

**Lesson:** For a cross-compiler targeting constrained platforms, static composition via includes (or compile-time file selection) is simpler and more predictable than runtime dispatch or dynamic linking.

**Recommendation for Rock:**
- Use `compile.sh` (or equivalent) to select which `.c` files to compile per target
- Keep the RTL as a set of `.c`/`.h` files that the generator can reference
- The generator already has `lib_path` and target awareness (from session 28) — extend this to select RTL source files per target

### 3. Mixed High-Level / Assembly Split

**Observed:** PASTA/80 puts arithmetic, string primitives, and heap allocation in assembly (performance-critical, called thousands of times) but keeps file buffering, command-line parsing, and higher-level logic in Pascal (maintainability matters, called infrequently).

**Lesson:** The 80/20 rule applies strongly on Z80. A small number of assembly routines (string ops, integer math, memory copy) account for most of the runtime's hot paths. Everything else can stay in the high-level language without measurable penalty.

**Recommendation for Rock:**
- Rock's existing split (C for most RTL, with `@embed asm` for Z80-specific hot paths) is already correct
- Candidates for Z80 assembly in Rock's RTL: string copy/compare, integer multiply/divide (if not using SDCC's builtins), array element access, memory fill/copy
- Keep array management, string formatting, and I/O in C — maintainability outweighs the marginal speed gain

### 4. Compiler-Built-In vs RTL-Defined Functions

**Observed:** PASTA/80 has ~30 "magic" built-ins (marked `[All*]`) that the compiler generates inline code for: `Read`, `Write`, `Assert`, `Inc`, `Dec`, `FillChar`, `SizeOf`, etc. These aren't defined anywhere in the RTL source — the compiler emits specialised code for each call site.

**Lesson:** Some operations are inherently polymorphic or need compile-time information (like `SizeOf` or format-string parsing for `Write`). These belong in the compiler, not the RTL.

**Recommendation for Rock:**
- Rock already does this for `print()`, array operations, and type conversions — this is the right approach
- Keep the boundary clear: if a function needs type information or generates different code per call site, it's a compiler built-in. If it's the same code every time, it belongs in the RTL.

---

## Patterns to Improve On

### 5. Error Handling via Global State

**Observed:** PASTA/80 uses a single `LastError: Byte` global. Every `Block*` function checks `if LastError <> 0 then Exit` at entry. This is simple but creates invisible cascading failures — if you forget to check `IOResult`, errors silently propagate and disable all subsequent file operations.

**Lesson:** Global error state is fragile. It works in single-threaded Pascal programs but doesn't compose well.

**Recommendation for Rock:**
- Use return values for error signalling, not global state
- Rock's C backend makes this natural: functions return error codes, or use a `rock_result_t` struct
- For the ZXN target where code size matters, a global `last_error` is acceptable as a pragmatic compromise, but document the cascading-skip semantics explicitly

### 6. Heap Allocator Without Coalescing

**Observed:** `__freemem` inserts freed blocks at the head of the free list without attempting to merge adjacent free blocks. This leads to progressive fragmentation.

**Lesson:** On a 64K machine with a ~32K heap, fragmentation is a real concern. Programs that allocate and free strings repeatedly will exhaust memory even when plenty is technically free.

**Recommendation for Rock:**
- Rock's existing `alloc.h`/`alloc.c` should be evaluated for coalescing behaviour
- At minimum, implement adjacent-block coalescing on free
- Consider a simple arena or pool allocator for strings and small arrays, with a general-purpose allocator as fallback
- The ZXN target especially needs a fragmentation-aware strategy

### 7. No Separation Between RTL and User Code at Build Time

**Observed:** The entire RTL is `{$i}`-included into every program. Combined with dead-code elimination (`--dep`), unused functions are stripped. But without `--dep`, the full RTL bloats every binary.

**Lesson:** Dependency analysis should be the default, not an opt-in flag. RTL code that isn't called shouldn't be linked.

**Recommendation for Rock:**
- Rock already benefits from C's compilation model — only referenced functions are linked
- For the ZXN target with SDCC, verify that unused RTL functions are actually stripped (SDCC's linker behaviour may differ from gcc)
- Consider splitting the RTL into multiple `.c` files so the linker can drop entire object files

### 8. 128-Byte Record Abstraction Leaks

**Observed:** The `Block*` HAL operates in 128-byte records (CP/M's native sector size). This works naturally for CP/M but requires awkward arithmetic in `FileSeek` for the Next and Agon targets, where the OS works in bytes. `FileSeek` in `files.pas` uses `Real` arithmetic (`P := 4.0 + I * 1.0 * CompSize`) to avoid integer overflow — a red flag that the abstraction level is wrong for non-CP/M platforms.

**Lesson:** Choose HAL abstractions that fit all targets, not just the first one. Byte-oriented I/O is more universal than record-oriented I/O.

**Recommendation for Rock:**
- Use byte-offset seeks in Rock's file HAL, not record-based
- If CP/M is ever a target, the CP/M HAL implementation can internally convert byte offsets to record numbers

---

## Architectural Recommendations for Rock's RTL

### Proposed Layer Structure

```
┌─────────────────────────────────────────────────┐
│ Layer 3: Rock Standard Library                  │
│ String formatting, array operations, I/O,       │
│ type conversions — written in Rock or C         │
│ (Generated by Rock compiler, consumes HAL)      │
├─────────────────────────────────────────────────┤
│ Layer 2: Platform HAL                           │
│ Per-target .c files selected at compile time    │
│ hal_host.c, hal_zxn.c, (future: hal_cpm.c)     │
│ ~15 functions: console, file, memory, timing    │
├─────────────────────────────────────────────────┤
│ Layer 1: Core C RTL                             │
│ fundefs_internal.c — array ops, string ops,     │
│ memory management, type system support          │
│ (Already exists in Rock, target-agnostic)       │
├─────────────────────────────────────────────────┤
│ Layer 0: Platform ASM (optional)                │
│ Z80 assembly for hot paths on ZXN target        │
│ (asm_interop.c/h already exists)                │
└─────────────────────────────────────────────────┘
```

### Key Design Decisions

1. **HAL granularity**: Byte-level I/O, not record-level. One `rock_file_read(handle, buf, count)` instead of block reads.

2. **Error model**: Return codes, not global state. `rock_result_t` or simple integer return codes.

3. **Build-time target selection**: `compile.sh` already selects gcc vs SDCC. Extend to select `hal_host.c` vs `hal_zxn.c`.

4. **Assembly integration**: Use `@embed asm` blocks for Z80 hot paths. Keep the C fallback for host target testing.

5. **Code size budget**: On ZXN, every byte of RTL reduces heap/stack space. Profile RTL size after each addition. PASTA/80's author notes RTL size as a known concern.

6. **Conditional compilation**: Rock's generator can emit `#ifdef TARGET_ZXN` guards, similar to PASTA/80's `{$ifdef SYS_ZXNEXT}`. Use sparingly — prefer separate files over ifdef forests.

### Priority Order for RTL Development

1. **Console I/O** — `print()` already works; formalise the HAL boundary
2. **String operations** — Already in `fundefs_internal.c`; verify ZXN compatibility
3. **Array operations** — Already in `fundefs_internal.c`; verify ZXN compatibility
4. **File I/O** — New capability; implement HAL for host first, then ZXN (esxDOS)
5. **Keyboard input** — `ReadKey`, `KeyPressed` equivalents
6. **Graphics** — ZXN-specific (Layer 2, sprites, etc.)
7. **Sound** — ZXN-specific (AY chip)

---

## Summary Table

| PASTA/80 Pattern | Quality | Rock Action |
|------------------|---------|-------------|
| Block* HAL | Adopt | Define ~15-function C HAL |
| Layered include composition | Adopt | Per-target file selection in compile.sh |
| Mixed Pascal/ASM split | Adopt | C + `@embed asm` for Z80 hot paths |
| Compiler built-ins for polymorphic ops | Adopt | Already doing this correctly |
| Global error state | Improve | Use return codes |
| No heap coalescing | Improve | Add adjacent-block merging |
| Mandatory dead-code elimination | Improve | Ensure linker strips unused RTL on all targets |
| 128-byte record I/O abstraction | Improve | Use byte-oriented I/O in HAL |
| Conditional compilation for minor diffs | Adopt (sparingly) | Prefer separate files; use `#ifdef` for constants only |

## See Also

- [[pasta80/pasta80-overview]] — Project overview
- [[pasta80/pasta80-rtl-architecture]] — Detailed architecture analysis
- [[pasta80/pasta80-target-platforms]] — Per-platform implementation details
