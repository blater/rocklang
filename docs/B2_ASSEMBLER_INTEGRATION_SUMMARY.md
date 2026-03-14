# B2: Assembler Integration - Complete Plan Summary

## Overview

**B2** enables Rock to seamlessly integrate with Z80 assembly code on ZX Spectrum Next, supporting direct hardware interaction, external assembly functions, and inline assembly blocks.

**Status**: Planning Complete ✓
**Phases**: 4 (approved); 1 skipped (memory-mapped I/O)
**Total Effort**: ~7-8 hours
**Risk Level**: Low-to-Medium

---

## Approved Phases

### Phase 1: Memory Access Functions (peek/poke) — 45 minutes
**Plan File**: `PHASE1_PEEK_POKE_PLAN.md`

Direct byte-level memory reading and writing.

**Deliverables**:
- `lib/external/asm_interop.h/c` (4 files)
- `test/peek_poke_test.rkr`
- `examples/memory_inspect.rkr`
- Updated `Makefile` and fundefs.h includes

**Functions**:
```rock
byte peek(word address);
void poke(word address, byte val);
```

**Effort**: 45 min | **Risk**: Very Low

---

### Phase 4: Inline Assembly (@embed asm) — 90 minutes
**Plan File**: `PHASE4_EMBED_ASM_PLAN.md`

Embed Z80 assembly directly within Rock code.

**Deliverables**:
- Parser changes to recognize `@embed asm ... @end asm`
- Generator changes to emit `__asm__()` blocks
- `test/inline_asm_test.rkr`
- `examples/port_io.rkr`

**Syntax**:
```rock
@embed asm
  ld a, 0xFF
  out (0xFE), a
@end asm
```

**Effort**: 90 min | **Risk**: Low

---

### Phase 3: External Assembly Functions — 4 hours
**Plan File**: `PHASE3_EXTERN_ASM_PLAN.md`

Call external .asm files from Rock code.

**Deliverables**:
- Parser: `extern sub` keyword support
- Generator: External function declarations
- Build system: `--asm-sources` CLI flag and Makefile integration
- `test/extern_asm_test.rkr`
- `examples/keyboard.rkr`
- `asm/keyboard.asm` and `asm/sound.asm` examples

**Syntax**:
```rock
extern sub scan_keyboard(): word;
extern sub play_sound(freq: word, duration: byte): void;
```

**Effort**: 4 hrs | **Risk**: Medium-High

---

### Phase 5: Fastcall Conventions — 60 minutes
**Plan File**: `PHASE5_FASTCALL_PLAN.md`

Document and demonstrate register-based parameter passing for performance.

**Deliverables**:
- `test/fastcall_test.rkr`
- `examples/performance_comparison.rkr`
- `FASTCALL_GUIDE.md` (comprehensive guide)
- Assembly examples showing register mapping

**Concept** (Z88DK automatically handles this):
- Parameters in L, H, E, D, C, B registers
- Returns in H:L (word) or L (byte)
- No stack overhead
- 3-5x faster than stack-based calls

**Effort**: 60 min | **Risk**: Very Low (documentation + examples only)

---

## Skipped Phase

### Phase 2: Memory-Mapped Variables (@mem) — DEFERRED
**Reason**: Z80 uses port-mapped I/O, not memory-mapped.

**Alternative approach**: Use `peek()/poke()` + external assembly for hardware I/O.

Can be revisited if needed for specific use cases.

---

## Recommended Implementation Order

```
1. Phase 1: peek/poke         (Day 1 morning - 45 min)
   ↓
2. Phase 4: @embed asm        (Day 1 morning - 90 min)
   ↓
3. Phase 3: extern asm        (Day 1 afternoon - 4 hours)
   ↓
4. Phase 5: fastcall          (Day 2 morning - 60 min)
```

**Rationale**:
- Phase 1 & 4 are isolated, low-risk foundational work
- Phase 3 builds on 1 & 4, highest value but most complex
- Phase 5 is pure documentation, can finish quickly

---

## Key Files Summary

### Create (New)
```
lib/external/asm_interop.h
lib/external/asm_interop.c
src/generation/asm_interop.h
src/generation/asm_interop.c
test/peek_poke_test.rkr
test/inline_asm_test.rkr
test/extern_asm_test.rkr
test/fastcall_test.rkr
examples/memory_inspect.rkr
examples/port_io.rkr
examples/keyboard.rkr
examples/performance_comparison.rkr
asm/keyboard.asm
asm/sound.asm
asm/fast_filter.asm
FASTCALL_GUIDE.md
```

### Modify (Existing)
```
lib/external/fundefs.h          (add include)
src/generation/fundefs.h        (add include)
src/parser.c                    (add @embed asm, extern support)
src/generator.c                 (add ASM generation)
src/generator.h                 (add embed_asm node type if needed)
src/main.c                      (add --asm-sources flag)
Makefile                        (update generation, add ASM rules)
```

---

## Test & Example Matrix

| Phase | Test File | Example File | Assembly Support |
|-------|-----------|--------------|------------------|
| 1 | peek_poke_test.rkr | memory_inspect.rkr | None |
| 4 | inline_asm_test.rkr | port_io.rkr | None |
| 3 | extern_asm_test.rkr | keyboard.rkr | keyboard.asm, sound.asm |
| 5 | fastcall_test.rkr | performance_comparison.rkr | fast_filter.asm |

---

## Integration Points

### Parser (`src/parser.c`)
- Recognize `@embed asm` ... `@end asm`
- Support `extern` keyword for function declarations
- Store assembly code and external flag in AST

### Generator (`src/generator.c`)
- Emit `__asm__()` blocks for inline assembly
- Generate forward declarations for external functions
- Skip body generation for `extern` functions
- Wrap assembly in `#ifdef __SDCC` guards

### Build System (`Makefile`)
- Accept `ASM_SOURCES` variable
- Pass `.asm` files to z88dk linker
- Update copy rules for asm_interop.h/c

### CLI (`src/main.c`)
- Accept `--asm-sources=file1.asm,file2.asm` flag
- Pass to build system or Makefile

---

## Host vs ZXN Target

### Host Target
- Phase 1: `peek()/poke()` are stubbed (return 0/no-op)
- Phase 4: `@embed asm` blocks are skipped with warning
- Phase 3: `extern` functions cannot be linked (expected - no assembly)
- Phase 5: Documentation applies conceptually

### ZXN Target
- Phase 1: `peek()/poke()` use direct pointer dereference
- Phase 4: `@embed asm` blocks included via `__asm__()`
- Phase 3: Assembly files linked with z88dk
- Phase 5: Register-based parameter passing used automatically

---

## Risk Assessment Summary

| Phase | Risk | Mitigation |
|-------|------|-----------|
| 1 | Very Low | Simple wrappers, proven pattern (`#ifdef __SDCC`) |
| 4 | Low | Parser-only change, straightforward text emission |
| 3 | Medium-High | Requires build system knowledge, linker integration |
| 5 | Very Low | Documentation + examples, no code changes |

---

## Success Criteria

### Phase 1 (peek/poke)
✅ Functions accessible from Rock code
✅ Host target returns 0 (stubbed)
✅ ZXN target uses pointer dereference
✅ Test compiles and runs

### Phase 4 (@embed asm)
✅ Parser accepts `@embed asm` blocks
✅ Generator emits `__asm__()` correctly
✅ Host target: Assembly skipped
✅ ZXN target: Assembly included

### Phase 3 (extern asm)
✅ Parser recognizes `extern sub`
✅ Generator emits forward declarations
✅ Assembly files compile and link
✅ Function calls work with correct parameters
✅ Symbol resolution successful

### Phase 5 (fastcall)
✅ Examples demonstrate register mapping
✅ Guide explains when/how to use fastcall
✅ Performance benefits documented
✅ Assembly examples for 1-6 parameters

---

## Dependencies

```
Phase 1 ────────────────────────┐
                                ├─→ Phase 3 (can depend on 1 & 4)
Phase 4 ────────────────────────┤
                                ├─→ Phase 5 (depends on Phase 3)
                                └──→ Integration Testing
```

- Phase 1 & 4 are independent
- Phase 3 can use concepts from 1 & 4
- Phase 5 documents Phase 3 usage

---

## Memory & Context

Each phase has its own detailed plan file:
1. `PHASE1_PEEK_POKE_PLAN.md` — Implementation details
2. `PHASE4_EMBED_ASM_PLAN.md` — Parser/generator changes
3. `PHASE3_EXTERN_ASM_PLAN.md` — Build system integration
4. `PHASE5_FASTCALL_PLAN.md` — Documentation + examples

Reference these for specific implementation details.

---

## Next Steps

1. **Review**: Ensure all phases align with vision
2. **Approve**: Give go-ahead to start Phase 1
3. **Clarify**: Any specific hardware features needed?
4. **Confirm**: Timeline and resource availability

---

## Appendix: Glossary

- **Fastcall**: Register-based calling convention (Z88DK)
- **Port-mapped I/O**: Z80 hardware accessed via `IN`/`OUT` instructions
- **sccz80**: Small C compiler (z88dk's C compiler for Z80)
- **__SDCC**: Preprocessor define set by z88dk (not SDCC)
- **Volatile**: C qualifier preventing optimization of memory access
- **Symbol resolution**: Linker matching function names to implementations
- **Name mangling**: Adding `_` prefix for C compatibility

---

## Questions?

See individual phase plans for details:
- **What's the parser change?** → PHASE4_EMBED_ASM_PLAN.md
- **How do I link assembly?** → PHASE3_EXTERN_ASM_PLAN.md
- **How fast is fastcall?** → PHASE5_FASTCALL_PLAN.md
- **How do peek/poke work?** → PHASE1_PEEK_POKE_PLAN.md
