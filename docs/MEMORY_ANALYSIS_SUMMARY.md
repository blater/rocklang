# String Memory Leak Analysis - Summary

## Quick Facts

| Issue | Severity | Type | Scope |
|-------|----------|------|-------|
| Intermediate concat results accumulate | 🔴 HIGH | Leak | Long-running programs, loops |
| String literal copies | 🟠 MEDIUM | Leak | Any print of literals in loops |
| `get_abs_path()` off-by-one | 🔴 CRITICAL | Bug | File operations |
| String array reassignment | 🟡 MEDIUM | Leak | Array updates |

---

## The Core Problem

Rock allocates **all strings in persistent memory** that is only freed at program exit:

```c
// Every string goes here:
string new_string(string s) {
  string res;
  res.data = allocate_compiler_persistent(s.length + 1);  // ← PERSISTENT!
  // ...
  return res;
}
```

When you do string operations, intermediate results pile up:

```rock
let res: string => "";                    // Alloc 1: 1 byte
res => concat(res, "hello");              // Alloc 2: 6 bytes → Alloc 1 UNREACHABLE
res => concat(res, " world");             // Alloc 3: 12 bytes → Alloc 2 UNREACHABLE
// At this point: Allocs 1 and 2 are in the persistent tracker but unusable
```

---

## Real-World Leak Examples

### Example 1: string_of_int() - The Worst Offender

```rock
// In lib/stdlib.rkr
let string_of_int(n: int): string {
    let res: string => "";
    // Loop: concat one digit at a time
    loop i: 0 -> length(chars) - 1 =>
        res => concat(res, get(chars, ...));
    return res;
}

// Usage in a loop:
loop i: 0 -> 100000 =>
    print(string_of_int(i));
```

**Memory usage**:
- Each `string_of_int(i)` creates ~log10(i) intermediate allocations
- Example for number 12345:
  - Concat '5': allocate 2 bytes
  - Concat '4': allocate 3 bytes, old 2 bytes lost
  - Concat '3': allocate 4 bytes, old 3 bytes lost
  - Concat '2': allocate 5 bytes, old 4 bytes lost
  - Concat '1': allocate 6 bytes, old 5 bytes lost
  - **Leaked in just this one number**: 2+3+4+5 = 14 bytes
  - **For 100K iterations with avg 4-digit numbers**: ~100KB leak

### Example 2: String Literal Copies in Loops

```rock
loop i: 0 -> 10000 =>
    print("value: ");
    print(toString(i));
    print("\n");
```

**Generated C code**:
```c
for (int i = 0; i <= 10000; i++) {
  print(new_string((string){.data = "value: ", .length = 7}));  // 8 bytes alloc
  print(toString(i));                                            // ~20 bytes alloc
  print(new_string((string){.data = "\n", .length = 1}));       // 2 bytes alloc
}
```

**Leak**: 8 + 20 + 2 = 30 bytes per iteration × 10,000 = **300 KB leak**

### Example 3: String Array Reassignment

```rock
let names: string[100] := [];
loop i: 0 -> 100 =>
    names[i] := "initial";          // Allocate
    names[i] := "replacement";      // New alloc, old one leaked
```

**Leak**: 100 × "initial".length = 700 bytes leaked (plus overhead)

---

## Allocation Overhead Hidden Costs

The persistent allocator tracks each allocation:

```c
typedef struct alloc_stack_t {
  alloc_elem_t *data;     // Array of allocations
  void** persistents;     // Pointers to allocated memory
  // ...
} alloc_stack_t;
```

Each allocation adds:
- 1 entry in the `persistents` array (~8 bytes on 64-bit)
- 1 entry in tracking array (~16 bytes)
- **Total tracking overhead**: ~24 bytes per string allocation

So a 10-byte string actually costs: 10 (data) + 24 (tracking) = **34 bytes per allocation**.

---

## Critical Bug Found & Fixed

### get_abs_path() - Line 104

**BEFORE** (WRONG):
```c
char *abs_path = allocate_compiler(strlen(abs_path_tmp + 1));
                                           // ↑ pointer arithmetic error
```

**AFTER** (FIXED):
```c
char *abs_path = allocate_compiler(strlen(abs_path_tmp) + 1);
                                                        // ↑ correct
```

This bug caused:
1. Allocating way too little memory (reading past pointer start)
2. Likely buffer overflow / segfault
3. Secondary leak when copying with `new_string()`

**Status**: ✅ FIXED in both `src/generation/fundefs.c` and `lib/external/fundefs.c`

---

## Why This Matters

### Short Programs (< 1 second)
- ✅ Not a problem - allocations freed at program exit
- Examples: `hello_world`, `fizz_buzz`, `array_test` all work fine

### Long-Running Programs (> 1 minute)
- ❌ Real memory leak - process grows indefinitely
- Examples:
  - Server accepting requests
  - Data processing pipeline with millions of items
  - Real-time systems

### Example: Impact on Processing 1 Million Items

```rock
let process_items(): void => {
    let items: string[] => read_all_items();  // 1M items
    loop i: 0 -> length(items) =>
        let result: string => concat(items[i], "_processed");
        print(result);
}
```

**Memory leak**:
- 1M × string copies = ~100MB
- Plus concat intermediates = another ~50MB
- **Total**: ~150MB leaked for 1M items
- Could trigger OOM on memory-constrained systems

---

## Severity Assessment

### For Current Users
- **Low Impact**: All existing examples work correctly (leak happens at program end)
- **Tests Pass**: hello_world, fizz_buzz, array_test, concat_test, array_records - all verified

### For Future Use
- **High Risk**: Any long-running program with string operations
- **High Risk**: Tight loops with `string_of_int()` / `toString()`
- **High Risk**: String-heavy data processing

---

## Solutions (Priority Order)

### Immediate (Done)
- ✅ Fix `get_abs_path()` off-by-one error

### Short Term (Recommended)
1. Refactor `string_of_int()` to use scope-based allocation and only persist final result
2. Refactor `create_string()` similarly
3. Document that strings are persistent-allocated

### Medium Term (Good)
4. Implement reference counting for strings:
   ```c
   typedef struct {
     char *data;
     size_t length;
     int refcount;  // NEW
   } string;
   ```

### Long Term (Best)
5. Redesign for linear types or auto-scope tracking

---

## Verification

**Build Status**: ✅ Clean compile, 0 errors/warnings
**Test Status**: ✅ All 5 regression tests passing
**Bug Fix**: ✅ `get_abs_path()` corrected

No behavioral changes - all leaks are latent, won't surface until long-running programs.

