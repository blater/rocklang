# B2: Assembler Integration - Complete Planning Documentation

## Quick Navigation

### Summary & Overview
- **`B2_ASSEMBLER_INTEGRATION_SUMMARY.md`** — Start here! High-level overview of all phases
  - Approved phases (1, 4, 3, 5)
  - Skipped phase (2 - memory-mapped I/O)
  - Timeline and dependencies
  - Risk assessment
  - Success criteria

### Individual Phase Plans
Each phase has a detailed implementation plan with test cases and examples:

1. **`PHASE1_PEEK_POKE_PLAN.md`** (45 minutes, Very Low Risk)
   - Direct memory read/write: `peek()` / `poke()`
   - New files: `lib/external/asm_interop.h/c`
   - Test: `test/peek_poke_test.rkr`
   - Example: `examples/memory_inspect.rkr`
   - *No parser/generator changes*

2. **`PHASE4_EMBED_ASM_PLAN.md`** (90 minutes, Low Risk)
   - Inline assembly: `@embed asm ... @end asm`
   - Parser changes: Recognize assembly blocks
   - Generator changes: Emit `__asm__()` directives
   - Test: `test/inline_asm_test.rkr`
   - Example: `examples/port_io.rkr`

3. **`PHASE3_EXTERN_ASM_PLAN.md`** (4 hours, Medium-High Risk)
   - External assembly functions: `extern sub`
   - Parser: Support `extern` keyword
   - Generator: Forward declarations only
   - Build system: Link `.asm` files via z88dk
   - CLI flag: `--asm-sources=file1.asm,file2.asm`
   - Test: `test/extern_asm_test.rkr`
   - Example: `examples/keyboard.rkr`
   - Assembly examples: `asm/keyboard.asm`, `asm/sound.asm`

4. **`PHASE5_FASTCALL_PLAN.md`** (60 minutes, Very Low Risk)
   - Register-based parameter passing (Z80 fastcall)
   - Documentation: `FASTCALL_GUIDE.md`
   - Test: `test/fastcall_test.rkr`
   - Example: `examples/performance_comparison.rkr`
   - Assembly example: `asm/fast_filter.asm`
   - *Pure documentation - no compiler changes*

---

## Implementation Roadmap

### Recommended Order
```
Phase 1  → Phase 4  → Phase 3  → Phase 5
(45min)    (90min)    (4hrs)     (1hr)
  ║          ║           ║         ║
  ╠──────────╨───────────╨─────────╨─→ Total: ~7-8 hours
```

**Why this order?**
- Phases 1 & 4 are isolated, low-risk foundations
- Phase 3 builds on 1 & 4 for high value
- Phase 5 documents Phase 3, minimal effort

### Phase Dependencies
```
Phase 1 (peek/poke)
├─ No dependencies
└─ Can start immediately

Phase 4 (@embed asm)
├─ No dependencies
└─ Can do in parallel with Phase 1, or after

Phase 3 (extern asm)
├─ Depends on: Understanding from Phases 1 & 4
├─ Most complex phase
└─ Requires build system knowledge

Phase 5 (fastcall)
├─ Depends on: Phase 3 (for context)
├─ Pure documentation
└─ Can overlap with Phase 3
```

---

## Files to Create (by Phase)

### Phase 1 New Files
```
lib/external/asm_interop.h
lib/external/asm_interop.c
src/generation/asm_interop.h
src/generation/asm_interop.c
test/peek_poke_test.rkr
examples/memory_inspect.rkr
```

### Phase 4 Modified Files (no new files)
```
src/parser.c (add ASM parsing)
src/generator.c (add ASM generation)
```

### Phase 3 New Files
```
test/extern_asm_test.rkr
examples/keyboard.rkr
asm/keyboard.asm
asm/sound.asm
```

### Phase 3 Modified Files
```
src/parser.c (extend)
src/generator.c (extend)
src/main.c (add --asm-sources flag)
Makefile (add ASM linker rules)
```

### Phase 5 New Files
```
test/fastcall_test.rkr
examples/performance_comparison.rkr
asm/fast_filter.asm
FASTCALL_GUIDE.md
```

---

## Host vs ZXN Target

### Phase 1: peek/poke
| Target | Behavior |
|--------|----------|
| Host | Stub functions return 0 / no-op |
| ZXN | Direct pointer dereference via volatile cast |

### Phase 4: @embed asm
| Target | Behavior |
|--------|----------|
| Host | Assembly blocks skipped with warning |
| ZXN | Inline assembly via `__asm__()` macro |

### Phase 3: extern asm
| Target | Behavior |
|--------|----------|
| Host | Cannot link (no assembly available) |
| ZXN | Links with z88dk's assembler |

### Phase 5: fastcall
| Target | Behavior |
|--------|----------|
| Host | Conceptual (normal function calls) |
| ZXN | Register-based parameter passing (automatic) |

---

## Test & Example Structure

Each phase includes:
- **Test file** (test/): Unit test validating functionality
- **Example file** (examples/): Practical, documented example
- **Assembly files** (asm/): For phases 3 & 5

All examples and tests should:
- Be valid, runnable Rock code
- Demonstrate the feature clearly
- Include comments explaining usage
- Work on both host and ZXN targets (with graceful degradation)

---

## Success Validation

### Phase 1: peek/poke
- ✅ Functions declared in asm_interop.h
- ✅ Included via fundefs.h
- ✅ Host target: Stubbed (returns 0)
- ✅ ZXN target: Uses volatile pointer cast
- ✅ `make bootstrap` passes
- ✅ peek_poke_test.rkr compiles and runs

### Phase 4: @embed asm
- ✅ Parser recognizes `@embed asm`
- ✅ AST node created for assembly
- ✅ Generator emits `__asm__()` blocks
- ✅ Host: Assembly skipped
- ✅ ZXN: Assembly included in C output
- ✅ inline_asm_test.rkr compiles and runs

### Phase 3: extern asm
- ✅ Parser accepts `extern sub`
- ✅ Generator emits forward declarations
- ✅ Linker resolves assembly symbols
- ✅ extern_asm_test.rkr compiles and links
- ✅ Function calls work with correct parameters

### Phase 5: fastcall
- ✅ Examples demonstrate register mapping
- ✅ Guide is clear and comprehensive
- ✅ Performance benefits documented
- ✅ All parameter counts (1-6) covered

---

## Risk & Timeline Summary

| Phase | Hours | Risk | Complexity |
|-------|-------|------|------------|
| 1 | 0.75 | Very Low | Trivial |
| 4 | 1.5 | Low | Simple |
| 3 | 4 | Medium-High | Complex |
| 5 | 1 | Very Low | Trivial |
| **Total** | **7.25** | | |

---

## Getting Started

### Before You Start
1. Read `B2_ASSEMBLER_INTEGRATION_SUMMARY.md` (10 min)
2. Understand the 4 approved phases
3. Review Phase 1 plan details (`PHASE1_PEEK_POKE_PLAN.md`)

### Start Phase 1
1. Create `lib/external/asm_interop.h/c`
2. Create `src/generation/asm_interop.h/c` mirrors
3. Update `lib/external/fundefs.h` include
4. Update `src/generation/fundefs.h` include
5. Create test and example
6. Run `make bootstrap` and validate

### Then Continue
Each phase stands alone. Follow the order, or skip ahead if you prefer.

---

## Document Map

```
B2_README.md (this file)
│
├── B2_ASSEMBLER_INTEGRATION_SUMMARY.md
│   └── High-level overview and coordination
│
├── PHASE1_PEEK_POKE_PLAN.md
│   ├── asm_interop.h/c implementation
│   ├── test/peek_poke_test.rkr
│   └── examples/memory_inspect.rkr
│
├── PHASE4_EMBED_ASM_PLAN.md
│   ├── Parser and generator changes
│   ├── test/inline_asm_test.rkr
│   └── examples/port_io.rkr
│
├── PHASE3_EXTERN_ASM_PLAN.md
│   ├── Parser, generator, build system
│   ├── test/extern_asm_test.rkr
│   ├── examples/keyboard.rkr
│   ├── asm/keyboard.asm
│   └── asm/sound.asm
│
└── PHASE5_FASTCALL_PLAN.md
    ├── FASTCALL_GUIDE.md
    ├── test/fastcall_test.rkr
    ├── examples/performance_comparison.rkr
    └── asm/fast_filter.asm
```

---

## Questions?

Each plan has its own detailed implementation guide. Start with the summary, then dive into individual phases as needed.

**Key files**:
- Overview: `B2_ASSEMBLER_INTEGRATION_SUMMARY.md`
- Implementation: Phase-specific .md files
- Reference: This README

**Ready to begin?** Start with Phase 1! 🚀
