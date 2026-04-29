#!/bin/bash
# Negative test runner.
#
# Each test in this directory is a Rock program that MUST fail compilation,
# emitting the diagnostic listed in the EXPECT_DIAG associative array below.
#
# A negative test passes if:
#   1. rockc exits non-zero AND
#   2. rockc's combined output contains the expected diagnostic substring.

set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ROCKC="$ROOT/rockc"

pass=0
fail=0

run_neg() {
  local file="$1" expect="$2"
  local out exit_code
  out="$($ROCKC "$file" /tmp/neg_out.exe 2>&1)"
  exit_code=$?
  rm -f /tmp/neg_out.exe /tmp/neg_out.exe.c

  if [ "$exit_code" -eq 0 ]; then
    echo "  FAIL $file — compiled successfully (expected failure)"
    fail=$((fail+1))
    return
  fi
  if ! echo "$out" | grep -qF "$expect"; then
    echo "  FAIL $file — exit non-zero but missing expected diagnostic"
    echo "    expected substring: $expect"
    echo "    actual output:"
    echo "$out" | sed 's/^/      /'
    fail=$((fail+1))
    return
  fi
  echo "  ok $(basename "$file")"
  pass=$((pass+1))
}

cd "$ROOT"

run_neg test/negative/recursive_record_via_array.rkr \
        "recursive type definition 'Node' forbidden"

run_neg test/negative/recursive_record_direct.rkr \
        "recursive type definition 'Loop' forbidden"

run_neg test/negative/mutual_recursion.rkr \
        "recursive type definition 'A' forbidden"

echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
