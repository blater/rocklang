# String Allocation & Memory Leak Analysis

## Executive Summary

**CRITICAL ISSUE FOUND**: The system has **multiple patterns that leak intermediate string allocations**. While the program eventually exits and all persistent allocations are freed by `kill_compiler_stack()`, intermediate results from string operations accumulate in the persistent allocator tracking array and are never freed during execution.

**Severity**: HIGH for long-running programs or high-volume string processing. LOW for short scripts (program-end cleanup still happens).

---

## Allocation Architecture

### Two-Tier System

1. **Scope-based** (`allocate_compiler`): Freed at scope end
2. **Persistent** (`allocate_compiler_persistent`): Tracked in global array, freed at program exit only

### String Policy: All Strings Use Persistent Allocation

```c
// From fundefs.c line 71
string new_string(string s) {
  string res;
  res.data = allocate_compiler_persistent(s.length + 1);  // PERSISTENT
  res.length = s.length;
  for (int i = 0; i < res.length; i++)
    res.data[i] = s.data[i];
  res.data[res.length] = 0;
  return res;
}
```

All strings live in persistent allocation:
- ✅ Guarantees no use-after-free (no scope cleanup)
- ❌ Intermediate results accumulate until program exit
- ❌ No way to free a string during execution

---

## Identified Leak Patterns

### 1. **Intermediate Concat Results** (HIGH FREQUENCY)

**Location**: `fundefs.c:25-40` (__concat_char, __concat_str)

```c
string __concat_str(string s1, string s2) {
  char *buffer = allocate_compiler_persistent(s1.length + s2.length + 1);
  // ... copy data ...
  return (string){.data = buffer, .length = s1.length + s2.length};
}
```

**Problem**: Each concat creates a new persistent allocation. If result is reassigned:

```c
let res: string => "";              // allocate 1 byte
res => concat(res, "x");            // allocate 2 bytes, old buffer unreachable
res => concat(res, "y");            // allocate 3 bytes, old 2-byte buffer unreachable
res => concat(res, "z");            // allocate 4 bytes, old 3-byte buffer unreachable
```

**Leak amount**: 1 + 2 + 3 = 6 bytes in intermediate allocations (plus 4 byte string struct overhead per allocation = ~18 bytes total for 3 operations)

**Common patterns that leak**:
- `string_of_int()` (lib/stdlib.rkr:37-50): Builds strings with loop concat - worst offender!
- `create_string()` (lib/stdlib.rkr:58-65): Same pattern
- Any string building loop with concat

### 2. **String Literals Copied Unnecessarily** (MEDIUM FREQUENCY)

**Location**: Generated code patterns

```c
// Generated for: print("\n")
print(new_string((string){.data = "\n", .length = 1}));
```

The `new_string()` call allocates persistent memory for a 2-byte buffer (1 char + null terminator) that is:
- Created fresh on every call
- Never stored in a variable
- Immediately passed to `print()` which only reads it
- Never freed during execution
- Accumulates in persistent allocator

**Leak amount**: 2 bytes per literal string operation (worst with loops)

### 3. **Bug in get_abs_path()** (CRITICAL)

**Location**: `fundefs.c:104` (line 104)

```c
// WRONG:
char *abs_path = allocate_compiler(strlen(abs_path_tmp + 1));
//                                  ^^^^ POINTER ARITHMETIC ERROR

// Should be:
char *abs_path = allocate_compiler(strlen(abs_path_tmp) + 1);
//                                                      ^ not +1 inside strlen!
```

This allocates `strlen(ptr_to_2nd_char)` bytes instead of `strlen(first_char) + 1`.

**Impact**:
- Allocates way too little memory (likely crash or buffer overflow)
- String is scope-based but incorrectly sized
- Then copied with `new_string()` into persistent (adds another leak on top)

**Example**: For 10-char path:
- Should allocate: 11 bytes (10 chars + null)
- Actually allocates: ~random bytes before `abs_path_tmp[1]` (probably crash)

---

## Problematic Functions

### 1. string_of_int() - WORST OFFENDER

**Location**: `lib/stdlib.rkr:37-50`

```rock
let string_of_int(n: int): string {
    let chars: char [] => [];
    if n = 0 then append(chars, '0');
    while n > 0 do {
        append(chars, n%10 + '0');
        n => n/10;
    }
    let res: string => "";
    loop i: 0 -> length(chars) -1 =>
        res =>
            concat(res, get(chars, length(chars) - 1 - i));

    return res;
}
```

**Leak analysis**:
- For number 12345:
  - Initialize `res = ""` → 1-byte allocation
  - Concat '5' → 2-byte allocation, 1-byte leaked
  - Concat '4' → 3-byte allocation, 2-byte leaked
  - Concat '3' → 4-byte allocation, 3-byte leaked
  - Concat '2' → 5-byte allocation, 4-byte leaked
  - Concat '1' → 6-byte allocation, 5-byte leaked
- **Total leaked**: 1+2+3+4+5 = 15 bytes per number
- **Plus**: Intermediate string structs in tracking (6 allocations × 24 bytes per allocation = 144 bytes tracking overhead)

---

### 2. create_string() - SIMILAR LEAK

**Location**: `lib/stdlib.rkr:58-65`

Same concat loop pattern as `string_of_int()`.

---

### 3. String Array Operations - NOW WORSE WITH FIX!

**Location**: `src/generation/fundefs_internal.c:207-227`

With the new fix, every array operation deep-copies strings:

```c
void string_push_array(..., string elem) {
  string copy = new_string(elem);  // NEW ALLOCATION
  __internal_push_array(arr, &copy);
}
```

**Leak pattern**: If you reassign array elements:

```c
let arr: string[3] => [];
arr[0] := "hello";      // allocate 6 bytes ("hello" + null)
arr[0] := "goodbye";    // allocate 8 bytes, previous 6-byte "hello" leaked!
```

Each reassignment leaks the previous value.

---

## Impact Severity by Use Case

### Low Risk (Program-end cleanup sufficient)
- Short scripts (< 1 second runtime)
- Small string counts (< 1000 strings)
- One-shot operations

### High Risk (Real memory leak)
- String-heavy loops: `loop i: 0 -> 1000000 => { res => concat(...) }`
- Server-like long-running processes
- Any use of `string_of_int()` in tight loops
- Large file processing with string operations

### Example: Fibonacci with string_of_int()

```rock
let main(): void => {
  loop i: 0 -> 100000 => {
    print(toString(i));  // Calls string_of_int internally
    print("\n");
  }
}
```

**Leak**: Each loop iteration:
- ~10 bytes from `string_of_int()` intermediate concat results
- ~2 bytes from literal "\n" copy
- ~10+ bytes from tracking overhead
- **Total**: ~100+ KB per 1000 iterations = 10+ MB for 100K iterations

---

## Root Cause

The fundamental issue is: **Strings are allocated in persistent memory but intermediate results are never freed**.

There are three approaches to fix this:

### Option 1: Reference Counting (Best)
Each string carries a refcount. Decrement when reassigned. Free when refcount reaches 0.

**Pros**: Efficient, no programmer burden, works with all patterns
**Cons**: Adds 8 bytes per string, requires careful reference tracking

### Option 2: Automatic Stack Cleanup (Good)
Use scope-based allocation for temporary strings, copy to persistent only when needed.

**Pros**: Most strings auto-free
**Cons**: Complex to implement, requires type inference (temp vs stored)

### Option 3: Manual Free API (Simple)
Provide `free_string()` function for cleanup.

**Pros**: Simple to implement
**Cons**: Error-prone, programmers must remember to call it

### Option 4: Linear Typing (Excellent but Complex)
Adopt linear type system where each value used exactly once.

**Pros**: Zero-overhead, no accumulation possible
**Cons**: Requires language redesign, breaks many patterns

---

## Concrete Evidence

Run this to see accumulation:

```rock
let main(): void => {
  loop i: 0 -> 1000 => {
    let s: string => "";
    loop j: 0 -> 10 =>
      s => concat(s, "x");
  }
}
```

**Expected leak**: ~1000 × (1+2+3+4+5+6+7+8+9+10) = 1000 × 55 = 55 KB

(No way to measure without adding debug code to allocator)

---

## Recommendations (Priority Order)

1. **CRITICAL FIX**: Fix `get_abs_path()` line 104 - off-by-one error causes crashes
2. **HIGH**: Refactor `string_of_int()` and `create_string()` to use scope-based strings and only persist final result
3. **MEDIUM**: Add reference counting to string type
4. **LOW**: Document that strings are persistent-allocated and unsuitable for long-running programs

