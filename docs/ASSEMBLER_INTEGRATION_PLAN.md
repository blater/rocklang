# B2 Plan: Assembler Integration & Direct Memory Access

## Overview

Enable Rock to seamlessly interact with Z80 assembly code, supporting:
- Direct memory access (peek/poke)
- Memory-mapped variables (bind to assembler labels)
- Call external assembly routines from Rock code
- Call assembly routines from C embed code
- Fast calling conventions with register parameters/returns
- Inline assembly via `@embed asm` blocks

**Target Platform**: ZX Spectrum Next (z88dk + sccz80)
**Scope**: 4 independent but complementary parts
**Complexity**: Medium-High

---

## Part 1: Memory Access Functions (peek/poke)

### Goal
Provide direct byte-level memory access for hardware interaction and custom data structures.

### Implementation

**File**: `lib/external/fundefs.h` + `lib/external/fundefs.c` (mirror to src/generation/)

```c
// fundefs.h
byte peek(word address);           // Read byte from memory address
void poke(word address, byte val); // Write byte to memory address
word peek_word(word address);      // Read 16-bit word (little-endian)
void poke_word(word address, word val); // Write 16-bit word

// fundefs.c (Z88DK version)
byte peek(word address) {
#ifdef __SDCC
  return *(byte*)address;
#else
  return 0;  // Host: stub
#endif
}

void poke(word address, byte val) {
#ifdef __SDCC
  *(byte*)address = val;
#else
  // Host: no-op
#endif
}
```

### Rock Usage Example
```rock
sub main(): void => {
  // Read byte from screen memory (0x4000 on ZX Spectrum)
  dim screen_byte: byte := peek(to_word(0x4000));

  // Write to keyboard port (0xFE for ZX Spectrum)
  poke(to_word(0xFE), to_byte(0x00));

  // Read 16-bit value
  dim addr: word := peek_word(to_word(0x5000));
}
```

### Test Case
- `test/peek_poke_test.rkr` — Basic memory reads/writes

---

## Part 2: Memory-Mapped Variables

### Goal
Bind Rock variables directly to assembler-defined memory locations and labels.

### Syntax

New directive: `@mem address` or `@mem label`

```rock
// Bind to absolute address (0x4000 = screen memory)
@mem 0x4000
dim screen: byte[];

// Bind to assembler label
@mem keyboard_buffer
dim kbd_input: byte[];

// Bind scalar
@mem 0x8000
dim game_state: word;
```

### Implementation Strategy

**Parser** (`src/parser.c`):
- Recognize `@mem` directive before variable declarations
- Store memory address in AST node metadata

**Generator** (`src/generator.c`):
- For `@mem` declarations, emit pointer cast instead of allocation:
  ```c
  // INPUT:
  @mem 0x4000
  dim screen: byte[];

  // OUTPUT (C):
  byte* screen = (byte*)0x4000;
  ```
- For arrays, wrap in `__internal_dynamic_array_t` with fixed data pointer:
  ```c
  __internal_dynamic_array_t screen = {
    .data = (void*)0x4000,
    .length = 0,
    .capacity = 256,
    .elem_size = sizeof(byte),
    .max_capacity = 256  // fixed size
  };
  ```

**Validation**:
- Check `@mem` only used on variable declarations
- Warn if address conflicts with known memory regions
- On host target: `@mem` variables are stubbed (no-op)

### Rock Usage Example
```rock
// Keyboard buffer at 0x5C00 (ZX Spectrum)
@mem 0x5C00
dim key_buffer: byte[];

sub read_key(): byte => {
  // Keyboard buffer is already at 0x5C00
  if length(key_buffer) > 0 {
    return get(key_buffer, 0);
  }
  return to_byte(0);
}
```

### Test Cases
- `test/memmap_scalar_test.rkr` — Bind simple word/dword to address
- `test/memmap_array_test.rkr` — Bind byte array to address

---

## Part 3: External Assembly Functions

### Goal
Call external `.asm` files from Rock code with proper linking and calling conventions.

### Syntax Extension

```rock
// Declare external assembly function (no implementation)
extern sub add_asm(a: byte, b: byte): byte;

// Use it like any other function
sub main(): void => {
  dim result: byte := add_asm(to_byte(3), to_byte(4));
  print(to_string(to_int(result)));
  print("\n");
}
```

### Assembly File Format (`keyboard.asm`)

Z80 assembly with z88dk conventions:

```asm
;; keyboard.asm - Keyboard handling for ZX Spectrum Next
SECTION code_user

; @fastcall: L = a, H = b, return in L
PUBLIC _add_asm
_add_asm:
  ; L contains first param, H contains second
  add a, h        ; A = L + H
  ld l, a         ; Return value in L
  ret

; Another example: scan keyboard
PUBLIC _scan_keyboard
_scan_keyboard:
  ; return keyboard state in H:L
  ld a, 0xFE      ; keyboard port
  in a, (0xFE)    ; read keyboard
  ld l, a
  ld h, 0
  ret
```

### Implementation

**Parser** (`src/parser.c`):
- Recognize `extern sub` keyword (already supported by some contexts)
- Mark function as external/unimplemented

**Generator** (`src/generator.c`):
- For external functions: emit forward declaration only, no body
- Generate fastcall compatible signatures:
  ```c
  // INPUT:
  extern sub multiply(a: byte, b: byte): byte;

  // OUTPUT (C):
  extern byte _multiply(byte a, byte b);  // wrapped by z88dk

  // CALL:
  byte result = _multiply(3, 4);
  ```

**Build System** (`Makefile`):
- Accept optional `ASM_SOURCES` variable
- Link z88dk compiled assembly with generated code
- Example:
  ```makefile
  BOOTSTRAP_OUTPUT.nex: out.c
    zcc +zxn -subtype=nex ... -clib=sdcc_iy \
        out.c src/generation/fundefs.c \
        keyboard.asm \
        -o output.nex
  ```

**Command-line Integration** (`src/main.c`):
- Accept `--asm-sources=file1.asm,file2.asm` flag
- Pass to zcc linker

### Rock Usage Example
```rock
extern sub scan_keyboard(): word;
extern sub play_sound(freq: word, duration: byte): void;

sub main(): void => {
  dim key: word := scan_keyboard();
  if key > 0 {
    play_sound(key, to_byte(100));
  }
}
```

### Test Cases
- `test/extern_asm_test.rkr` — Simple external function call
- `asm/keyboard.asm` — Example keyboard scanning routine
- `asm/sound.asm` — Example sound generation routine

---

## Part 4: Inline Assembly via @embed asm

### Goal
Allow direct Z80 assembly inline within Rock code for performance-critical sections.

### Syntax

```rock
sub main(): void => {
  @embed asm
    ; Direct Z80 assembly
    ld a, 0xFF       ; Load accumulator
    out (0xFE), a    ; Write to port
    nop
  @end asm
}
```

### Implementation

**Parser** (`src/parser.c`):
- Recognize `@embed asm` ... `@end asm` blocks
- Store raw assembly text in AST

**Generator** (`src/generator.c`):
- Emit assembly directly into generated C via `__asm__` inline assembly:
  ```c
  // INPUT:
  @embed asm
    ld a, 0xFF
    out (0xFE), a
  @end asm

  // OUTPUT (C):
  __asm__(
    "ld a, 0xFF\n"
    "out (0xFE), a\n"
  );
  ```

**Restrictions**:
- Only on host target with `#ifdef __SDCC`:
- On host: warning "assembly skipped on host target"

### Rock Usage Example
```rock
sub get_screen_address(): word => {
  dim addr: word := 0;
  @embed asm
    ld hl, 0x4000    ; Screen address
  @end asm
  return addr;
}
```

### Test Case
- `test/inline_asm_test.rkr` — Simple inline assembly

---

## Part 5: Fastcall Conventions

### Goal
Enable performance-optimized register-based parameter passing for assembly interop.

### Concept

Z88DK's `@fastcall` convention:
- Parameters passed in registers (L, H, E, D, etc.)
- Return value in H:L pair
- Faster than stack-based calling

### Implementation

**Annotation** (future extension):
```rock
// Annotate functions that should use fastcall
@fastcall
extern sub multiply_fast(a: byte, b: byte): word;
```

For now, this is Z88DK-specific and handled at assembly level. Rock can:
1. Document which functions expect fastcall
2. Let z88dk handle actual optimization
3. Future: Add compiler support for explicit register mapping

### Test Case
- `test/fastcall_test.rkr` — Performance comparison

---

## Implementation Roadmap

### Phase 1: peek/poke (Lowest Risk)
- [ ] Add `peek()`, `poke()` functions to fundefs.c/h
- [ ] Create `test/peek_poke_test.rkr`
- [ ] Test on host (stub) and ZXN
- **Effort**: 30 minutes
- **Risk**: Very low (isolated, clear semantics)

### Phase 2: @mem directive (Memory-mapped variables)
- [ ] Parser: Recognize `@mem address` directive
- [ ] AST: Store address metadata
- [ ] Generator: Emit pointer cast for `@mem` variables
- [ ] Generator: Wrap arrays in fixed `__internal_dynamic_array_t`
- [ ] Create test cases
- **Effort**: 2-3 hours
- **Risk**: Medium (requires parser/generator changes)

### Phase 3: External assembly functions
- [ ] Parser: Support `extern sub` declarations (if not already)
- [ ] Generator: Emit forward declarations for external functions
- [ ] Build: Update Makefile to accept `ASM_SOURCES`
- [ ] CLI: Add `--asm-sources` flag to main.c
- [ ] Create example assembly files (keyboard.asm, sound.asm)
- [ ] Create test case
- **Effort**: 3-4 hours
- **Risk**: Medium-High (linker integration, build system changes)

### Phase 4: @embed asm (Inline assembly)
- [ ] Parser: Recognize `@embed asm` blocks
- [ ] Generator: Emit via `__asm__` macro
- [ ] Create test case
- **Effort**: 1-2 hours
- **Risk**: Low (parser-only, straightforward emission)

### Phase 5: fastcall conventions (Nice-to-have)
- [ ] Document fastcall expectations
- [ ] Add `@fastcall` annotation (optional, doesn't affect codegen yet)
- [ ] Build example that demonstrates performance benefit
- **Effort**: 1 hour (documentation + example)
- **Risk**: Very low (no code changes needed, z88dk handles it)

---

## Files to Create/Modify

### New Files
```
lib/external/asm_interop.h     (new - asm function signatures)
lib/external/asm_interop.c     (new - stub implementations)
src/generation/asm_interop.h   (mirror)
src/generation/asm_interop.c   (mirror)
test/peek_poke_test.rkr
test/memmap_scalar_test.rkr
test/memmap_array_test.rkr
test/extern_asm_test.rkr
test/inline_asm_test.rkr
asm/keyboard.asm               (example)
asm/sound.asm                  (example)
```

### Modified Files
```
lib/external/fundefs.h         (+ peek, poke declarations)
lib/external/fundefs.c         (+ peek, poke implementations)
src/parser.c                   (+ @mem, @embed asm parsing)
src/generator.c                (+ @mem codegen, @embed asm emission, extern support)
src/main.c                     (+ --asm-sources CLI flag)
Makefile                       (+ ASM_SOURCES variable, linker rules)
```

---

## Example: Keyboard Scanning (Full Integration)

### assembly: `asm/keyboard.asm`
```asm
SECTION code_user
PUBLIC _scan_keyboard
_scan_keyboard:
  ld a, 0xFE
  in a, (0xFE)
  ld l, a
  ld h, 0
  ret
```

### Rock code: `test/keyboard_test.rkr`
```rock
extern sub scan_keyboard(): word;

@mem 0x5C00
dim key_buffer: byte[];

sub main(): void => {
  dim keys: word := scan_keyboard();
  print("Keyboard state: ");
  print(to_string(keys));
  print("\n");

  if peek(to_word(0xFE)) != to_byte(255) {
    print("Key pressed!\n");
  }
}
```

### Build
```bash
./bootstrap test/keyboard_test.rkr keyboard_test --target=zxn --asm-sources=asm/keyboard.asm
```

### Generated C (snippet)
```c
extern word _scan_keyboard(void);

byte* key_buffer = (byte*)0x5C00;

void main(void) {
  word keys = _scan_keyboard();
  print(__rock_make_string("Keyboard state: ", 16));
  print(to_string(keys));
  ...
}
```

---

## Risk Assessment

| Part | Risk | Mitigation |
|------|------|-----------|
| peek/poke | Very Low | Simple C wrappers, well-tested |
| @mem | Medium | Careful pointer casting, validation |
| extern asm | High | Requires linker knowledge, build integration |
| @embed asm | Low | Straightforward text emission |
| fastcall | Very Low | Documentation only (z88dk handles it) |

---

## Success Criteria

✅ **Phase 1 (peek/poke)**
- `peek()` reads byte from memory correctly
- `poke()` writes byte to memory correctly
- Host target: functions stubbed cleanly
- ZXN target: functions work via direct pointer access

✅ **Phase 2 (@mem)**
- Variables bound to memory addresses
- Arrays work with fixed capacity
- Assembly labels can be referenced
- No allocation overhead

✅ **Phase 3 (extern asm)**
- External .asm files compile and link
- Functions callable from Rock
- Multiple asm files supported
- Linker symbols resolve correctly

✅ **Phase 4 (@embed asm)**
- Inline assembly emits correctly
- Preprocessor understands syntax
- Host target: warning on skip

✅ **Phase 5 (fastcall)**
- Performance benchmarks show improvement
- Documentation clear and correct

---

## Next Steps

1. **Approve this plan** — Do the phases and timeline align with your vision?
2. **Prioritize** — Start with Phase 1 (peek/poke) for quick win?
3. **Clarify** — Any specific keyboard/sound routines you want to target first?
4. **Scope adjustments** — Any features to add/remove?
