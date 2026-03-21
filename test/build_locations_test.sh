#!/bin/bash

# Test rockc can resolve includes correctly from different build locations
# This verifies that include paths are resolved relative to the source file,
# not relative to the current working directory

set -e

ROCK_ROOT="/Users/blater/src/rock"
ROCKC="$ROCK_ROOT/rockc"
INPUT="$ROCK_ROOT/test/simple_test.rkr"

# Create temporary build directories
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

echo "Testing rockc include path resolution..."
echo ""

# Test 1: Run from project root with relative path
echo "[Test 1] Run from project root with relative path"
cd "$ROCK_ROOT"
OUTPUT1="$TMPDIR/test1_output"
if $ROCKC test/simple_test.rkr "$OUTPUT1" > /dev/null 2>&1; then
  echo "✓ PASS: simple_test.rkr compiled successfully from project root"
else
  echo "✗ FAIL: simple_test.rkr failed to compile from project root"
  exit 1
fi

# Test 2: Run from subdirectory with absolute path
echo "[Test 2] Run from subdirectory with absolute path"
SUBDIR="$TMPDIR/subdir"
mkdir -p "$SUBDIR"
cd "$SUBDIR"
OUTPUT2="$TMPDIR/test2_output"
if $ROCKC "$INPUT" "$OUTPUT2" > /dev/null 2>&1; then
  echo "✓ PASS: simple_test.rkr compiled successfully from subdirectory"
else
  echo "✗ FAIL: simple_test.rkr failed to compile from subdirectory"
  exit 1
fi

# Test 3: Run from completely different directory with absolute path
echo "[Test 3] Run from /tmp with absolute path"
cd /tmp
OUTPUT3="$TMPDIR/test3_output"
if $ROCKC "$INPUT" "$OUTPUT3" > /dev/null 2>&1; then
  echo "✓ PASS: simple_test.rkr compiled successfully from /tmp"
else
  echo "✗ FAIL: simple_test.rkr failed to compile from /tmp"
  exit 1
fi

echo ""
echo "All tests passed! ✓"
