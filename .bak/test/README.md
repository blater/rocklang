# Rock Test Suite

This directory contains all unit and functional tests for the Rock compiler.

## Running Tests

From the project root, run:

```bash
./run_tests.sh
```

This will:
- Compile each test file
- Execute the compiled test
- Report pass/fail status
- Summarize results

## Test Files

All test files follow the naming convention: `*_test.rkr`

### Core Type Tests
- `array_test.rkr` - Array creation, append, indexing, length
- `byte_test.rkr` - Byte type (unsigned 8-bit) operations
- `word_test.rkr` - Word type (unsigned 16-bit) operations
- `dword_test.rkr` - Dword type (unsigned 32-bit) operations
- `length_test.rkr` - length() function for strings and arrays

### String/Substring Tests
- `concat_test.rkr` - String concatenation
- `substring_test.rkr` - substring() function with various indices
- `substring_advanced_test.rkr` - Edge cases and advanced usage
- `substring_oob_test.rkr` - Out-of-bounds error testing (expected failure)
- `tostring_test.rkr` - to_string() conversion for various types

### Memory & Allocation Tests
- `memory_test.rkr` - Memory allocation and cleanup
- `assign_test.rkr` - Variable assignment operations

### Type Conversion Tests
- `byte_advanced_test.rkr` - Advanced byte operations and casting
- `format_test.rkr` - Function arguments with arrays and type conversion

### Embedding Tests
- `embed_test.rkr` - Inline C code embedding with function calls
- `embed_inline_test.rkr` - Inline C embedding
- `embed_simple_test.rkr` - Simple C code embedding

### Error Handling Tests
- `error_test.rkr` - Error condition testing (expected failure)
- `format_test.rkr` - String formatting (expected failure)
- `enum_test.rkr` - Enum type testing

## Expected Failures

The following tests are expected to fail or skip:
- `error_test.rkr` - Tests compiler error messages
- `substring_oob_test.rkr` - Tests out-of-bounds error handling

## Adding New Tests

To add a new test:

1. Create a file in `test/` named `your_feature_test.rkr`
2. Implement the test with appropriate assertions via print statements
3. Run `./run_tests.sh` to verify it works
4. Add documentation above if it tests a new feature

## Test Output

Each test should produce minimal, deterministic output that can be visually verified or compared against expected output. Use:

```rock
print("✓ Feature test passed\n");
```

for success output, or error messages for failures.
