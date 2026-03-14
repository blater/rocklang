# Plan: Move `_val` Wrappers to Internal Header

## Problem
The `_val` wrapper functions (`__concat_str_val`, `__to_string_int_val`, etc.) are exposed in the public header (fundefs.h) when they should be internal compiler-generated helpers only.

## Solution
Move wrappers from public fundefs.h to internal fundefs_internal.h, rename with `__rock_internal_` prefix, and update generator to use the new names.

## Files to Modify

### 1. lib/cpu_agnostic/fundefs.h
- **Remove:** Lines 46-102 (all `_val` wrapper declarations guarded with `#ifndef __SDCC`)
- Keep all public API declarations

### 2. lib/cpu_agnostic/fundefs_internal.h
- **Add:** All 12 `_val` wrappers from fundefs.h, renamed:
  - `__rock_make_string_val` → `__rock_internal_make_string_val`
  - `new_string_val` → `__rock_internal_new_string_val`
  - `cstr_to_string_val` → `__rock_internal_cstr_to_string_val`
  - `__concat_char_val` → `__rock_internal_concat_char_val`
  - `__concat_str_val` → `__rock_internal_concat_str_val`
  - `__substring_from_val` → `__rock_internal_substring_from_val`
  - `__substring_range_val` → `__rock_internal_substring_range_val`
  - `__to_string_byte_val` → `__rock_internal_to_string_byte_val`
  - `__to_string_int_val` → `__rock_internal_to_string_int_val`
  - `__to_string_word_val` → `__rock_internal_to_string_word_val`
  - `__to_string_dword_val` → `__rock_internal_to_string_dword_val`
- Keep `#ifndef __SDCC` guard

### 3. src/generator.c
Update function calls to use renamed wrappers:
- `emit_concat_host()`: Change `__concat_char_val`, `__concat_str_val` to internal names
- `emit_to_string_host()`: Change all `__to_string_*_val` to internal names (4 variants)
- `emit_substring_host()`: Change `__substring_from_val`, `__substring_range_val` to internal names
- `emit_string_literal_host()`: Change `__rock_make_string_val` to internal name

Total: ~12 rename points

### 4. lib/external/fundefs.h
- **Remove:** Same wrapper declarations as cpu_agnostic version

### 5. lib/external/fundefs_internal.h
- **Add:** Same 12 renamed wrappers as cpu_agnostic version

## Validation

After changes:
1. `make clean && make bootstrap` - Verify compilation
2. Run all HOST tests: concat_test, tostring_test, array_test, substring_test, byte_test, word_test, dword_test
3. Verify no linker errors

## Rollback
If any compilation, linking, or test failure occurs:
```bash
git checkout lib/ src/generator.c
```
