# B2 Phase 4: Inline Assembly via @embed asm

## Goal
Allow developers to embed Z80 assembly code directly within Rock code for performance-critical sections and low-level hardware interaction.

## Scope
- New syntax: `@embed asm ... @end asm` blocks
- Emits raw Z80 assembly into generated C via `__asm__()` macro
- Host target: Warning on skip (assembly blocks ignored)
- ZXN target: Assembly inserted inline via sccz80 inline asm syntax

## Syntax

```rock
sub main(): void => {
  // Inline assembly block
  @embed asm
    ; Z80 assembly code goes here
    ld a, 0xFF       ; Load accumulator with 0xFF
    out (0xFE), a    ; Write to keyboard port
    nop
    nop
  @end asm
}
```

## Implementation

### Parser Changes (`src/parser.c`)

**New AST node type**: `embed_asm` (parallel to existing `embed` node)

```c
typedef enum ast_tag_t {
  // ... existing tags ...
  embed,           // @embed c ... @end c
  embed_asm,       // @embed asm ... @end asm  (NEW)
} ast_tag_t;

// In AST union, add:
typedef struct {
  string_view lang;      // "asm"
  string_view code;      // raw assembly text
} embed_asm_t;

// Add to union:
embed_asm_t embed_asm;
```

**Parser logic** (in `parse_statement`):

```c
// Check for @embed directive
if (sv_eq(tok.lexeme, sv("@embed"))) {
  advance();
  string_view lang = tok.lexeme;
  advance();

  if (sv_eq(lang, sv("asm"))) {
    // New: parse assembly block
    string_view asm_code = sv(""); // accumulate lines
    while (!sv_eq(tok.lexeme, sv("@end"))) {
      asm_code = sv_concat(asm_code, tok.lexeme);
      advance();
    }
    advance(); // consume @end
    advance(); // consume asm

    ast_t node = allocate_compiler(sizeof(ast_t));
    node->tag = embed_asm;
    node->data.embed_asm.lang = sv("asm");
    node->data.embed_asm.code = asm_code;
    return node;
  }
  else if (sv_eq(lang, sv("c"))) {
    // Existing C embed code
    // ...
  }
}
```

### Generator Changes (`src/generator.c`)

**In `generate_statement`**, add handler for `embed_asm`:

```c
else if (stmt->tag == embed_asm) {
  generate_embed_asm(g, stmt);
}
```

**New function** `generate_embed_asm`:

```c
void generate_embed_asm(generator_t *g, ast_t node) {
  FILE *f = g->f;
  string_view code = node->data.embed_asm.code;

  fprintf(f, "#ifdef __SDCC\n");
  fprintf(f, "  __asm__(\n");

  // Emit assembly lines as quoted strings
  // Handle line-by-line to preserve comments
  for (size_t i = 0; i < code.length; i++) {
    fprintf(f, "    \"");
    // Emit characters, escaping quotes and newlines
    while (i < code.length && code.data[i] != '\n') {
      if (code.data[i] == '"') fprintf(f, "\\\"");
      else if (code.data[i] == '\\') fprintf(f, "\\\\");
      else fprintf(f, "%c", code.data[i]);
      i++;
    }
    fprintf(f, "\\n\"\n");
  }

  fprintf(f, "  );\n");
  fprintf(f, "#else\n");
  fprintf(f, "  // Assembly block skipped on host target\n");
  fprintf(f, "#endif\n");
}
```

### Example Output

**Rock code**:
```rock
@embed asm
  ld a, 0xFF
  out (0xFE), a
@end asm
```

**Generated C** (ZXN target):
```c
#ifdef __SDCC
  __asm__(
    "ld a, 0xFF\n"
    "out (0xFE), a\n"
  );
#else
  // Assembly block skipped on host target
#endif
```

## Test Case: `test/inline_asm_test.rkr`

```rock
sub main(): void => {
  print("Testing inline assembly\n");

  // Test 1: Simple no-op instruction
  @embed asm
    nop
    nop
  @end asm
  print("Executed NOP instructions\n");

  // Test 2: Load and output
  @embed asm
    ld a, 0x42       ; Load 0x42 into accumulator
    nop
  @end asm
  print("Executed LD and NOP\n");

  // Test 3: Multiple lines
  @embed asm
    ld a, 0xFF
    ld b, 0x00
    add a, b
  @end asm
  print("Executed multi-line assembly\n");

  print("Inline assembly test complete\n");
}
```

### Expected Output (Host)
```
Testing inline assembly
Executed NOP instructions
Executed LD and NOP
Executed multi-line assembly
Inline assembly test complete
```

*(Assembly blocks are skipped with #else on host)*

### Expected Output (ZXN)
```
Testing inline assembly
Executed NOP instructions
Executed LD and NOP
Executed multi-line assembly
Inline assembly test complete
```

*(Assembly executes; output is from print statements)*

## Example: `examples/port_io.rkr`

A practical example showing direct port I/O via inline assembly.

```rock
// Port I/O example
// Demonstrates direct hardware access via inline assembly

sub read_keyboard_row(row: byte): byte => {
  dim result: byte := 0;

  @embed asm
    ; Read keyboard row
    ; Input: A = row number
    ; Output: A = keyboard state
    ld a, l              ; L = row parameter
    ld bc, 0xFEFE        ; BC = keyboard port
    in a, (c)            ; Read from port
    ld l, a              ; Return in L
  @end asm

  return result;
}

sub write_port(port: byte, value: byte): void => {
  @embed asm
    ; Write to port
    ; L = port, H = value
    ld b, l              ; B = port
    ld c, 0xFE           ; C = 0xFE (port I/O)
    ld a, h              ; A = value
    out (c), a           ; Output to port
  @end asm
}

sub main(): void => {
  print("=== Port I/O Example ===\n");

  // Example 1: Read keyboard port
  dim key_state: byte := read_keyboard_row(to_byte(0));
  print("Keyboard row 0 state: ");
  print(to_string(to_int(key_state)));
  print("\n");

  // Example 2: Write to sound port
  write_port(to_byte(0xFE), to_byte(0x10));
  print("Wrote to sound port\n");

  print("Done\n");
}
```

## Compiler Integration

### Parser
- Extends `parse_statement()` to recognize `@embed asm`
- Accumulates raw assembly text until `@end asm`
- Stores in AST node with `embed_asm` tag

### Generator
- New `generate_embed_asm()` function
- Wraps assembly in `__asm__()` for sccz80
- Guards with `#ifdef __SDCC` to skip on host
- Preserves comments and formatting

### Build Process
1. Parser creates `embed_asm` AST nodes
2. Generator emits inline asm directives
3. sccz80 compiles with `-c` (C output)
4. z88dk's z80 assembler processes inline asm
5. Linking happens normally

## Testing Strategy

### Host Target
```bash
./bootstrap test/inline_asm_test.rkr inline_asm_test
./inline_asm_test
# Output: All print statements execute, assembly skipped silently
```

### ZXN Target
```bash
./bootstrap test/inline_asm_test.rkr inline_asm_test --target=zxn
# C code contains __asm__() blocks
# sccz80 processes them
```

## Validation Checklist

- [ ] Parser recognizes `@embed asm ... @end asm` syntax
- [ ] AST `embed_asm` node created and populated
- [ ] Generator `generate_embed_asm()` function implemented
- [ ] `#ifdef __SDCC` guards assembly on correct targets
- [ ] `__asm__()` syntax is valid for sccz80
- [ ] Comments in assembly are preserved
- [ ] Newlines and indentation are correct
- [ ] `make bootstrap` builds cleanly
- [ ] inline_asm_test.rkr compiles and runs
- [ ] Host target: Assembly blocks skipped, prints work
- [ ] ZXN target: Assembly blocks included in C output

## Risk Assessment

**Risk Level**: Low

**Why**:
- Parser-only changes (isolated to parse_statement)
- Generator is straightforward text emission
- No AST changes to other node types
- Host fallback is clear (skip assembly)
- Proven pattern (already have `@embed c` working)

**Potential Issues**:
- String escaping (quotes, backslashes in assembly)
  - **Mitigation**: Careful character-by-character emission
- Line preservation and alignment
  - **Mitigation**: Emit each line as separate string in __asm__()
- sccz80's inline asm syntax variations
  - **Mitigation**: Test with z88dk documentation

## Success Criteria

✅ Parser accepts `@embed asm ... @end asm` blocks
✅ Generator emits valid `__asm__()` code
✅ Host target: Assembly skipped cleanly
✅ ZXN target: Assembly included in C output
✅ Test compiles and executes on both targets
✅ Comments and formatting preserved in output
✅ No new warnings or errors

## Timeline

**Estimated effort**: 90 minutes
- 30 min: Add AST node type and parser logic
- 30 min: Implement generate_embed_asm() function
- 15 min: Create test case
- 15 min: Create example
- 5 min: Verify build and test

## Dependencies

- Phase 1 (peek/poke) not required
- Can be done independently
- Prepares for Phase 3 (extern asm functions)

## Next Step

Once Phase 4 is complete, move to **Phase 3: External Assembly Functions** (linking .asm files).
