# Fix: Relative Path Resolution in rockc

## Problem Statement

rockc fails when invoked from a directory other than the project root because include paths are resolved relative to the current working directory (CWD) instead of relative to the project root.

### Current Behavior (BROKEN)
- Test 1 ✓: `cd ~/src/rock && ./rockc test/simple_test.rkr` → Works (CWD = project root)
- Test 2 ✗: `cd /tmp/subdir && /path/to/rockc /path/to/test/simple_test.rkr` → Fails (CWD ≠ project root)
- Test 3 ✗: `cd /tmp && /path/to/rockc /path/to/test/simple_test.rkr` → Fails (CWD ≠ project root)

### Root Cause

When rockc processes `include "test/Assert.rkr"`:
1. `parser.c:get_abs_path()` calls `realpath("test/Assert.rkr")`
2. `realpath()` resolves relative to CWD, not to the input file's location
3. If CWD ≠ project root, include resolution fails

## Solution Design

**Key Insight:** The project root can be inferred from the input file's location. All includes should be resolved relative to this computed project root, not relative to CWD.

### Step 1: Compute Project Root in main.c

When an input file is provided:
- Resolve input to absolute path: `/Users/blater/src/rock/test/simple_test.rkr`
- Extract directory: `/Users/blater/src/rock/test`
- Go up one level to project root: `/Users/blater/src/rock`
- Store this as `projectRoot`

**Rationale:** The project root is where `test/`, `src/`, `lib/` directories reside. Since input files are typically in `test/` subdirectory, we go up one level from the input file's directory.

### Step 2: Pass Project Root to Parser

- Add `set_include_base_dir(projectRoot)` call in `main.c` before parsing
- This sets the base directory for include resolution in the parser

### Step 3: Update Include Resolution in parser.c

Modify `get_abs_path()` to:
1. If path is absolute, resolve it directly (no change needed)
2. If path is relative and `include_base_dir` is set:
   - Construct: `include_base_dir + "/" + relative_path`
   - Then call `realpath()` on the constructed path
3. If path is relative and `include_base_dir` is not set:
   - Fall back to current behavior (resolve from CWD)

## Implementation Steps

### Step 1: main.c - Add Project Root Computation

```c
// After converting input to absolute path
char *input_dir = dirname(input_copy);  // /Users/blater/src/rock/test
char *project_root = dirname(input_dir);  // /Users/blater/src/rock

// Pass to parser
set_include_base_dir(project_root);
```

**Files to modify:**
- `src/main.c`: Add project root computation and `set_include_base_dir()` call
- `src/parser.h`: Declare `set_include_base_dir()` function
- `src/parser.c`: Implement `set_include_base_dir()` and update `get_abs_path()`

### Step 2: parser.c - Global State for Project Root

```c
// Global variable (at file scope, after includes)
char *include_base_dir = NULL;

void set_include_base_dir(char *dir) {
  include_base_dir = dir;
}
```

### Step 3: parser.c - Update get_abs_path()

```c
char *get_abs_path(char *s) {
  char full_path[1024];

  // If path is relative and we have a base dir, resolve relative to it
  if (include_base_dir && s[0] != '/') {
    snprintf(full_path, sizeof(full_path), "%s/%s", include_base_dir, s);
  } else {
    strcpy(full_path, s);
  }

  // Resolve to absolute path
  char buffer[1024];
  memset(buffer, 0, 1024);
  realpath(full_path, buffer);
  int l = strlen(buffer);
  char *res = allocate_compiler_persistent(l + 1);
  strcpy(res, buffer);
  return res;
}
```

## Expected Test Results After Fix

**Test 1:** `cd ~/src/rock && ./rockc test/simple_test.rkr`
- Input: `test/simple_test.rkr` → abs path: `/Users/blater/src/rock/test/simple_test.rkr`
- Project root: `/Users/blater/src/rock`
- Include: `test/Assert.rkr` → resolves to `/Users/blater/src/rock/test/Assert.rkr` ✓

**Test 2:** `cd /tmp/subdir && /path/to/rockc /path/to/test/simple_test.rkr`
- Input: `/Users/blater/src/rock/test/simple_test.rkr` (absolute)
- Project root: `/Users/blater/src/rock`
- Include: `test/Assert.rkr` → resolves to `/Users/blater/src/rock/test/Assert.rkr` ✓

**Test 3:** `cd /tmp && /path/to/rockc /path/to/test/simple_test.rkr`
- Input: `/Users/blater/src/rock/test/simple_test.rkr` (absolute)
- Project root: `/Users/blater/src/rock`
- Include: `test/Assert.rkr` → resolves to `/Users/blater/src/rock/test/Assert.rkr` ✓

## Changes Summary

| File | Change |
|------|--------|
| `src/main.c` | Add project root computation; call `set_include_base_dir()` |
| `src/parser.h` | Add `set_include_base_dir()` declaration |
| `src/parser.c` | Add global `include_base_dir` variable; implement `set_include_base_dir()`; update `get_abs_path()` to use project root for relative paths |

## Verification

After implementation:
```bash
./test/build_locations_test.sh
```

Expected output:
```
✓ PASS: Test 1 (run from project root with relative path)
✓ PASS: Test 2 (run from subdirectory with absolute path)
✓ PASS: Test 3 (run from /tmp with absolute path)
All tests passed! ✓
```
