# B2 Phase 3: External Assembly Functions

## Goal
Enable linking and calling external Z80 assembly functions from Rock code, with proper symbol resolution and calling conventions.

## Scope
- `extern sub` declarations for assembly functions
- Compile `.asm` files separately with z88dk
- Link object files with generated C code
- Support multiple assembly files
- Fastcall calling convention compatibility

## Syntax

```rock
// Declare external function (no implementation)
extern sub scan_keyboard(): word;
extern sub play_sound(freq: word, duration: byte): void;

sub main(): void => {
  dim keys: word := scan_keyboard();
  if keys > 0 {
    play_sound(keys, to_byte(100));
  }
}
```

## Implementation

### Parser Changes (`src/parser.c`)

**Support `extern` keyword in function declarations**

The parser likely already supports forward declarations. We need to ensure `extern sub` is recognized:

```c
// In parse_function_declaration():
if (is_keyword("extern")) {
  advance();  // consume "extern"
  // Continue parsing normally
  // Set a flag on AST node: is_external = true
  fundef->is_external = 1;  // Add to ast_t if not present
}

// Then proceed with normal sub parsing
```

### Generator Changes (`src/generator.c`)

**Generate forward declarations for external functions**

```c
void generate_forward_defs(generator_t *g, ast_t program) {
  // ... existing code ...

  // Add new section for external functions
  fprintf(f, "// External assembly functions\n");
  for (int i = 0; i < stmts.length; i++) {
    ast_t stmt = stmts.data[i];
    if (stmt->tag == fundef && stmt->data.fundef.is_external) {
      generate_external_function_declaration(g, stmt);
    }
  }
}

void generate_external_function_declaration(generator_t *g, ast_t fundef) {
  FILE *f = g->f;
  fundef_t fdef = fundef->data.fundef;

  // Emit forward declaration
  generate_type(f, fdef.return_type);
  fprintf(f, " _");  // Prefix with _ for C name mangling
  fprintf(f, SV_Fmt, SV_Arg(fdef.name.lexeme));
  fprintf(f, "(");

  // Emit parameter list
  for (int i = 0; i < fdef.params.length; i++) {
    if (i > 0) fprintf(f, ", ");
    generate_type(f, fdef.params.data[i].type);
    fprintf(f, " ");
    fprintf(f, SV_Fmt, SV_Arg(fdef.params.data[i].name.lexeme));
  }

  fprintf(f, ");\n");  // No body for external functions
}
```

**Skip body generation for external functions**

```c
void generate_statement(generator_t *g, ast_t stmt) {
  // ... existing cases ...

  if (stmt->tag == fundef) {
    // Skip generation if external
    if (stmt->data.fundef.is_external) {
      return;  // Declaration already emitted in forward_defs
    }
    generate_fundef(g, stmt);
  }
}
```

### Build System Changes (`Makefile`)

**Add support for ASM_SOURCES variable**

```makefile
ASM_SOURCES ?=

# For ZXN target: compile assembly and link with generated code
%.nex: %.c
	@echo "Compiling for ZX Spectrum Next..."
	zcc +zxn -subtype=nex -startup=31 -clib=sdcc_iy -lndos \
	  -pragma-include:src/generation/zpragma_zxn.inc \
	  -create-app -o $* $< \
	  src/generation/fundefs.c \
	  src/generation/fundefs_internal.c \
	  $(ASM_SOURCES) \
	  2>&1
```

### CLI Changes (`src/main.c`)

**Add `--asm-sources` flag**

```c
// In main():
char *asm_sources = NULL;

for (int i = 1; i < argc; i++) {
  char *arg = argv[i];

  if (strncmp(arg, "--asm-sources=", 14) == 0) {
    asm_sources = arg + 14;  // Get value after =
  }
}

// After transpile, write asm sources to generated script or Makefile
if (asm_sources) {
  // Option A: Emit to comment in generated C
  fprintf(g.f, "/* ASM_SOURCES=%s */\n", asm_sources);

  // Option B: Write .mk file for Makefile inclusion
  FILE *mk = fopen("_asm.mk", "w");
  fprintf(mk, "ASM_SOURCES=%s\n", asm_sources);
  fclose(mk);
}
```

**Or simpler: Pass via environment or Makefile directly**

```bash
./bootstrap test/keyboard_test.rkr keyboard --target=zxn
# User then compiles:
ASM_SOURCES=asm/keyboard.asm make keyboard.nex
```

## Assembly File Format

Z80 assembly compatible with z88dk's z88dk toolchain.

### Example: `asm/keyboard.asm`

```asm
;; keyboard.asm - Keyboard reading for ZX Spectrum Next
;; Z80 assembly for z88dk

SECTION code_user

PUBLIC _scan_keyboard

; Read entire keyboard state
; Returns word (H:L) with keyboard bits
; L = low byte, H = high byte
_scan_keyboard:
  ; Read from keyboard port 0xFE
  ld a, 0xFE
  in a, (0xFE)
  ld l, a          ; Return in L
  ld h, 0          ; H = 0
  ret

PUBLIC _read_key_row

; Read specific keyboard row
; Parameters: L = row number
; Returns: A = keyboard state, L = result
_read_key_row:
  ; L contains row parameter
  ld a, l
  ld c, a          ; C = row (loop counter)
  ld b, 0xFE       ; B = port
  in a, (c)        ; Read from port
  ld l, a
  ld h, 0
  ret
```

### Example: `asm/sound.asm`

```asm
;; sound.asm - Sound generation for ZX Spectrum Next

SECTION code_user

PUBLIC _beep

; Generate a beep sound
; Parameters: L = frequency_low, H = frequency_high, E = duration_low, D = duration_high
_beep:
  ; Simple implementation: output to speaker port
  ld a, 0x10          ; Speaker output value
  ld b, 0xFE          ; Port address
  out (b), a          ; Output

  ; Delay loop (duration in DE)
  ld hl, 0
_beep_loop:
  add hl, de
  dec hl
  ld a, h
  or l
  jr nz, _beep_loop

  ret
```

## Test Case: `test/extern_asm_test.rkr`

```rock
// Test external assembly function calling

extern sub simple_add(a: byte, b: byte): byte;
extern sub get_value(): word;

sub main(): void => {
  print("Testing external assembly functions\n");

  // Test 1: Call function with parameters
  dim result1: byte := simple_add(to_byte(10), to_byte(20));
  print("simple_add(10, 20) = ");
  print(to_string(to_int(result1)));
  print("\n");

  // Test 2: Call function with return value
  dim value: word := get_value();
  print("get_value() = ");
  print(to_string(value));
  print("\n");

  print("External assembly test complete\n");
}
```

### Assembly Support: `asm/test_functions.asm`

```asm
SECTION code_user

PUBLIC _simple_add
PUBLIC _get_value

; simple_add: L = a, H = b, return in L
_simple_add:
  ld a, l
  add a, h
  ld l, a
  ret

; get_value: return 0x1234 in HL
_get_value:
  ld hl, 0x1234
  ret
```

### Expected Output (Host)
```
Testing external assembly functions
simple_add(10, 20) = 30
get_value() = 4660
External assembly test complete
```

### Expected Output (ZXN)
```
Testing external assembly functions
simple_add(10, 20) = 30
get_value() = 4660
External assembly test complete
```

## Example: `examples/keyboard.rkr`

A practical keyboard scanning example.

```rock
// Keyboard scanning example
// Demonstrates calling external assembly functions

extern sub scan_keyboard(): word;
extern sub read_key_row(row: byte): byte;

sub check_key_pressed(key_mask: byte): boolean => {
  dim state: byte := read_key_row(to_byte(0));

  // Check if any bits match the mask
  return state != to_byte(0);
}

sub main(): void => {
  print("=== Keyboard Scanner ===\n");
  print("Reading keyboard...\n");

  // Scan the keyboard
  dim keys: word := scan_keyboard();

  print("Keyboard state: 0x");
  print(to_string(keys));
  print("\n");

  // Check specific row
  dim row0: byte := read_key_row(to_byte(0));
  print("Row 0: 0x");
  print(to_string(to_int(row0)));
  print("\n");

  // Check if anything pressed
  if row0 != to_byte(255) {
    print("Key pressed detected!\n");
  } else {
    print("No keys pressed\n");
  }

  print("Done\n");
}
```

## Compiler Integration

### Parse Flow
1. Parser reads `extern sub` declarations
2. Sets `is_external` flag on AST node
3. Generator skips body generation
4. Forward declarations emitted in `generate_forward_defs()`

### Codegen Flow
1. `generate_forward_defs()` emits `extern` declarations
2. Function calls use normal calling convention
3. Linker resolves symbols from `.asm` files

### Build Flow
1. Rock code → generated C
2. User provides ASM_SOURCES variable
3. z88dk compiles both C and assembly
4. Linker combines object files
5. Output binary has both C and assembly code

## Usage

### Step 1: Write Rock code with `extern` declarations
```rock
extern sub my_function(x: byte): word;

sub main(): void => {
  dim result: word := my_function(to_byte(42));
}
```

### Step 2: Write matching assembly
```asm
PUBLIC _my_function
_my_function:
  ; L = parameter
  ld a, l
  ld h, 0
  ld l, a
  ret
```

### Step 3: Compile
```bash
./bootstrap myprogram.rkr myprogram --target=zxn
ASM_SOURCES=asm/myfunction.asm make myprogram.nex
```

Or with integrated flag:
```bash
./bootstrap myprogram.rkr myprogram --target=zxn --asm-sources=asm/myfunction.asm
# (future enhancement)
```

## Validation Checklist

- [ ] Parser recognizes `extern` keyword on function declarations
- [ ] `is_external` flag added to AST fundef node
- [ ] Generator emits forward declarations for external functions
- [ ] Generator skips body generation for external functions
- [ ] C code contains `extern` declarations (not implementations)
- [ ] Name mangling with `_` prefix matches z88dk conventions
- [ ] Assembly functions compile with z88dk
- [ ] Linker resolves assembly symbols correctly
- [ ] `make bootstrap` builds cleanly
- [ ] extern_asm_test.rkr compiles and links
- [ ] Host target: External declarations present but not called (would link error)
- [ ] ZXN target: Assembly functions link successfully

## Risk Assessment

**Risk Level**: Medium-High

**Why**:
- Involves build system changes
- Requires understanding linker behavior
- Z80 calling conventions must match
- Name mangling must be compatible with z88dk

**Mitigation**:
- z88dk's `_` prefix convention is well-documented
- Keep assembly files simple initially
- Test with known working examples
- Provide clear documentation and examples

## Success Criteria

✅ Parser accepts `extern sub` declarations
✅ Generator emits correct forward declarations
✅ Assembly files compile with z88dk
✅ Linker resolves all external symbols
✅ Function calls work with correct parameters
✅ Return values are correctly passed back
✅ Test compiles and executes
✅ Example demonstrates practical use
✅ No new warnings or errors

## Timeline

**Estimated effort**: 4 hours
- 30 min: Parser changes for `extern` recognition
- 45 min: AST modifications for `is_external` flag
- 45 min: Generator changes for external function handling
- 45 min: Makefile and CLI integration
- 30 min: Create test case and assembly
- 30 min: Create example
- 15 min: Verify build and linking

## Dependencies

- Phase 4 (@embed asm) not strictly required
- Phase 1 (peek/poke) not required
- Recommended to complete Phase 4 first for familiarity

## Known Challenges

1. **Name mangling**: Rock function names → C names with `_` prefix
   - **Solution**: Use consistent z88dk convention

2. **Calling conventions**: Z80 uses registers (L, H, E, D)
   - **Solution**: Document in assembly, validate with examples

3. **Type mapping**: Rock types → C types → Assembly conventions
   - **Solution**: Document type conversions, provide examples for each

4. **Symbol resolution**: Linker must find assembly symbols
   - **Solution**: Use `PUBLIC` declarations in assembly

## Next Step

Once Phase 3 is complete, move to **Phase 5: Fastcall Conventions** (documentation + examples).
