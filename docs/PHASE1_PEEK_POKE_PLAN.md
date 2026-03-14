# B2 Phase 1: Memory Access Functions (peek/poke)

## Goal
Provide byte-level direct memory read/write functions for hardware interaction and low-level programming on Z80 targets.

## Scope
- `peek(word address): byte` — Read single byte from memory
- `poke(word address, byte value): void` — Write single byte to memory
- Host target: Stub implementations (no-op)
- ZXN target: Direct pointer dereference via cast

## Implementation

### Files to Create
```
lib/external/asm_interop.h      (new - memory access declarations)
lib/external/asm_interop.c      (new - memory access implementations)
src/generation/asm_interop.h    (mirror)
src/generation/asm_interop.c    (mirror)
```

### Files to Modify
```
lib/external/fundefs.h          (add #include "asm_interop.h")
src/generation/fundefs.h        (add #include "asm_interop.h")
Makefile                        (update generation target to copy .c/.h)
```

### Code: `lib/external/asm_interop.h`
```c
#ifndef ROCKER_ASM_INTEROP_H
#define ROCKER_ASM_INTEROP_H

#include "typedefs.h"

// Direct memory access (Z80 assembly interoperability)
byte peek(word address);
void poke(word address, byte val);

#endif // ROCKER_ASM_INTEROP_H
```

### Code: `lib/external/asm_interop.c`
```c
#include "asm_interop.h"

byte peek(word address) {
#ifdef __SDCC
  // Z88DK: Direct pointer dereference
  return *(volatile byte*)address;
#else
  // Host: Stub (no actual memory access)
  (void)address;
  return 0;
#endif
}

void poke(word address, byte val) {
#ifdef __SDCC
  // Z88DK: Direct pointer write
  *(volatile byte*)address = val;
#else
  // Host: Stub (no-op)
  (void)address;
  (void)val;
#endif
}
```

### Integration

**Makefile** (update `create_generation` target):
```makefile
create_generation:
	@mkdir -p $(GEN)
	@if [ -d lib/external ]; then \
	  cp lib/external/*.h $(GEN); \
	  cp lib/external/*.c $(GEN); \
	  cp lib/external/*.inc $(GEN); \
	fi
```

**lib/external/fundefs.h** (add after other includes):
```c
#include "asm_interop.h"
```

**src/generation/fundefs.h** (add after other includes):
```c
#include "asm_interop.h"
```

## Test Case: `test/peek_poke_test.rkr`

```rock
sub main(): void => {
  print("Testing peek/poke functions\n");

  // Test 1: Basic poke and peek roundtrip
  // (We use a local variable's address as safe memory)
  dim test_val: byte := 0;
  print("Initial value: ");
  print(to_string(to_int(test_val)));
  print("\n");

  // Test 2: Poke a value
  poke(to_word(0), to_byte(42));
  print("After poke(0, 42)\n");

  // Test 3: Peek the value back
  dim read_val: byte := peek(to_word(0));
  print("Peeked value: ");
  print(to_string(to_int(read_val)));
  print("\n");

  // Test 4: Multiple consecutive pokes
  poke(to_word(10), to_byte(100));
  poke(to_word(11), to_byte(200));
  dim val1: byte := peek(to_word(10));
  dim val2: byte := peek(to_word(11));
  print("Peek(10)=");
  print(to_string(to_int(val1)));
  print(", Peek(11)=");
  print(to_string(to_int(val2)));
  print("\n");

  print("peek/poke test complete\n");
}
```

### Expected Output (Host)
```
Testing peek/poke functions
Initial value: 0
After poke(0, 42)
Peeked value: 0
Peek(10)=0, Peek(11)=0
peek/poke test complete
```

*(Host target stubs return 0; actual memory writes don't happen)*

### Expected Output (ZXN)
```
Testing peek/poke functions
Initial value: 0
After poke(0, 42)
Peeked value: 42
Peek(10)=100, Peek(11)=200
peek/poke test complete
```

## Example: `examples/memory_inspect.rkr`

A practical example showing how to inspect memory regions.

```rock
// Memory inspection tool
// Shows how peek() is used to read hardware memory areas

sub display_byte(addr: word, label: string): void => {
  dim val: byte := peek(addr);
  print(label);
  print(": 0x");
  print(to_string(to_int(val)));
  print("\n");
}

sub main(): void => {
  print("=== Z80 Memory Inspector ===\n");
  print("\nReading system variables (0x5C00+):\n");

  // SYSVAR area on ZX Spectrum
  display_byte(to_word(0x5C00), "KSTATE");
  display_byte(to_word(0x5C01), "LAST_K");
  display_byte(to_word(0x5C02), "DEBOUNCE");

  print("\nReading frame counter (0x5C78):\n");
  display_byte(to_word(0x5C78), "FRAMES");

  print("\nDone\n");
}
```

## Compiler Integration

### Generator Changes (`src/generator.c`)

No changes needed! The functions are declared in asm_interop.h and included via fundefs.h. They'll be available automatically once the files are in src/generation/.

### Build Process

1. Makefile creates_generation already copies .h and .c files
2. asm_interop.h/c get copied to src/generation/
3. fundefs.h includes asm_interop.h
4. Generated code automatically has access to peek/poke

## Testing Strategy

### Host Target
```bash
./bootstrap test/peek_poke_test.rkr peek_poke_test
./peek_poke_test
# Output: All zeros (stubbed)
```

### ZXN Target
```bash
./bootstrap test/peek_poke_test.rkr peek_poke_test --target=zxn
# Will compile with z88dk
# (Execution requires ZX Next emulator)
```

## Validation Checklist

- [ ] asm_interop.h/c created and mirrored to src/generation/
- [ ] fundefs.h includes asm_interop.h in both locations
- [ ] Makefile create_generation copies all .c files
- [ ] `make bootstrap` builds cleanly
- [ ] peek_poke_test.rkr compiles and runs on host
- [ ] memory_inspect.rkr example is valid Rock code
- [ ] Host target: peek/poke return 0 (stubbed)
- [ ] ZXN target: C code contains `*(volatile byte*)address` patterns

## Risk Assessment

**Risk Level**: Very Low

**Why**:
- Simple, isolated C wrappers
- No parser/generator changes needed
- Conditional compilation (`#ifdef __SDCC`) is proven pattern
- Fallback behavior clear on both targets

## Success Criteria

✅ Host target: Test compiles and runs, produces zeros
✅ ZXN target: C code generates with correct volatile pointer casts
✅ Functions accessible from Rock code via normal function call syntax
✅ No new warnings or errors in build

## Timeline

**Estimated effort**: 45 minutes
- 15 min: Create asm_interop.h/c (4 files)
- 10 min: Update Makefile and fundefs.h includes
- 10 min: Create test case
- 10 min: Create example
- 5 min: Verify build and test

## Next Step

Once Phase 1 is complete, move to **Phase 4: @embed asm** (inline assembly blocks).
