# Proposal: Inline C Code Support

## Goals

Allow Rock code to embed native C functions that:
- Receive Rock variables as parameters
- Access Rock scope variables
- Mutate Rock variables
- Return values to Rock code
- Interop with string, array, and record types

---

## Syntax Proposal

### Option A: `c_code` Block Statement

```rock
sub example(): void => {
  dim x: int => 5;
  dim y: int => c_code {
    return x * 2 + 1;
  };
  print(toString(y));  // prints "11"
}
```

**Pros**: Reads naturally, direct variable access
**Cons**: Requires scope variable injection into C

### Option B: `native` Function Definition

```rock
native multiply(a: int, b: int): int {
  return a * b;
}

sub test(): void => {
  print(toString(multiply(6, 7)));  // prints "42"
}
```

**Pros**: Explicit declaration, type-safe
**Cons**: No direct scope variable access

### Option C: `c!` Macro-Style (Recommended for MVP)

```rock
sub example(): void => {
  dim arr: int[] => [];
  append(arr, 10);

  c!{
    // Direct C code
    printf("Array length: %zu\n", arr->length);
    arr->data[0] = 99;
  }

  print(get(arr, 0));  // prints "99"
}
```

**Pros**: Simple parsing, clear intent, scope access
**Cons**: No type safety, dangerous

---

## Recommendation: Hybrid Approach

**Phase 1 (MVP)**: `native` functions only
- Type-safe
- Explicit interface
- Easy code generation
- No scope variable injection needed

**Phase 2**: `c!` block statements for variable access

---

## Phase 1: `native` Function Syntax

### Declaration

```rock
native function_name(param1: type1, param2: type2): return_type {
  C code here
}
```

### Type Mapping

| Rock Type | C Type | Notes |
|-----------|--------|-------|
| `int` | `int` | Direct |
| `char` | `char` | Direct |
| `boolean` | `int` | 0 = false, 1 = true |
| `string` | `string` (struct {char* data, size_t length}) | By value, immutable reference |
| `int[]` | `__internal_dynamic_array_t` | Opaque pointer to array |
| `record Foo` | `Foo` | By value (if small) or pointer |

### Examples

```rock
native strlen_custom(s: string): int {
  int count = 0;
  for (int i = 0; i < s.length; i++) {
    count++;
  }
  return count;
}

native double_number(x: int): int {
  return x * 2;
}

native array_sum(arr: int[]): int {
  int sum = 0;
  for (size_t i = 0; i < arr->length; i++) {
    sum += arr->data[i];
  }
  return sum;
}
```

---

## Phase 1 Implementation Strategy

### 1. Lexer/Parser
- Add `TOK_NATIVE` token for keyword `native`
- Parse `native func_name(params): return_type { c_code }`
- Store C code as string verbatim (include everything between `{` and `}`)

### 2. AST
- Add `ast_native_fn` node with:
  ```c
  struct {
    string name;
    ast_param_array params;
    string return_type;
    string c_code;
  }
  ```

### 3. Code Generation
- Generate C function signature from Rock declaration
- Inline C code verbatim
- Wrap in Rock function call in generated code

### Example Generation

**Rock code**:
```rock
native add(a: int, b: int): int {
  return a + b;
}
```

**Generated C**:
```c
int add(int a, int b) {
  return a + b;
}
```

### 4. Type Checking
- Validate param types are C-compatible
- Validate return type is C-compatible
- No validation of C code content (user responsibility)

---

## Phase 2: Scope Variable Access (`c!` blocks)

For accessing variables in scope:

```rock
sub example(): void => {
  dim x: int => 42;
  dim y: string => "hello";

  c!{
    // Access Rock variables directly
    printf("x = %d\n", x);
    printf("y = %s\n", y.data);

    // Mutate them
    x = 100;
    y.data[0] = 'H';
  }

  print(toString(x));  // prints "100"
}
```

**Implementation**:
- Parse C code block
- Inject variable declarations at top (copy from scope table)
- Inject mutation handling (copy back to scope if needed)
- Wrap in generated function scope

---

## Safety Considerations

### Phase 1 (native functions): Safe by design
- Type signatures enforced
- No scope access possible
- Bounds checking up to C code
- User writes C, bears responsibility

### Phase 2 (c! blocks): Less safe
- Direct variable access
- Possible to corrupt data
- Must document risks clearly
- Consider requiring `unsafe` keyword

---

## Limitations & Constraints

1. **C Code Only**: No other languages
2. **No Macro Expansion**: C code is literal, no Rock variables substituted
3. **Type Safety**: Limited - developer writes C, must match Rock types
4. **Portability**: C code must be portable across target platforms
5. **24KB Systems**: Inline C code counts toward total binary size

---

## File Changes Required

### Lexer (`src/lexer.c`)
- Add `native` keyword recognition

### Token (`src/token.h/c`)
- Add `TOK_NATIVE` (count currently 46)

### AST (`src/ast.h/c`)
- Add `ast_native_fn` node type
- Update `ast_kind` enum

### Parser (`src/parser.c`)
- Add `parse_native()` function
- Update `parse_statement()` to handle native
- Handle `{...}` block capture

### Generator (`src/generator.c`)
- Add `generate_native()` function
- C signature generation
- Code inlining

---

## Testing Strategy

### Phase 1 Test Cases

```rock
native add(a: int, b: int): int { return a + b; }
native concat_strings(s1: string, s2: string): string {
  // Return new concatenation
  ...
}
native array_get_first(arr: int[]): int {
  return arr->data[0];
}
```

### Expected Verification
- Builds without errors
- Functions callable from Rock code
- Correct parameter passing
- Correct return values

---

## Questions for User

1. **Phase 1 or both phases now?** (recommend Phase 1 first)
2. **Safety: Explicit `unsafe` keyword for c! blocks?**
3. **C Code Validation**: Any linting/checking, or full user responsibility?
4. **Calling Convention**: Stack vs register? (C compiler decides)
5. **Error Handling**: What if C code crashes?

---

## Success Criteria

✅ Can write C functions with Rock type interfaces
✅ Can call C functions from Rock code
✅ Type-safe function signatures
✅ No runtime overhead for type checking
✅ Survives 24KB memory constraints
✅ All existing tests still pass

