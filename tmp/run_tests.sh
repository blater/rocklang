#!/bin/bash

# Rock Test Runner
# Runs all tests in the test/ directory

PASS=0
FAIL=0
SKIP=0

echo "=== ROCK TEST SUITE ==="
echo ""

# Expected failures (tests that intentionally fail)
EXPECTED_FAILURES="error_test substring_oob_test"

for test in test/*_test.rkr; do
  name=$(basename "$test" .rkr)

  # Check if this test is expected to fail
  if echo "$EXPECTED_FAILURES" | grep -q "$name"; then
    SKIP=$((SKIP + 1))
    echo "⊘ $name (expected failure)"
    continue
  fi

  # Run test
  ./bootstrap "$test" "$name" > /dev/null 2>&1
  if [ $? -ne 0 ]; then
    FAIL=$((FAIL + 1))
    echo "✗ $name (compile failed)"
    continue
  fi

  if ./"$name" > /dev/null 2>&1; then
    PASS=$((PASS + 1))
    echo "✓ $name"
  else
    FAIL=$((FAIL + 1))
    echo "✗ $name (runtime failed)"
  fi
done

echo ""
echo "=== SUMMARY ==="
echo "Passed:  $PASS"
echo "Failed:  $FAIL"
echo "Skipped: $SKIP"
echo ""

if [ $FAIL -eq 0 ]; then
  echo "✅ All tests passed!"
  exit 0
else
  echo "❌ Some tests failed"
  exit 1
fi
