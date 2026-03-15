#!/bin/bash

# Rock compiler backend script
# Compiles generated C code for target platform
# Usage: compile.sh [--target=zxn|host] <output-name> <generated-c-file>

set -e

# Find the Rock repo root (parent of this script's directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROCK_ROOT="$(dirname "$SCRIPT_DIR")"

# Parse arguments
TARGET="host"
OUTPUT_NAME=""
GENERATED_C=""

for arg in "$@"; do
  if [[ "$arg" == --target=* ]]; then
    TARGET="${arg#--target=}"
  elif [[ -z "$OUTPUT_NAME" ]]; then
    OUTPUT_NAME="$arg"
  else
    GENERATED_C="$arg"
  fi
done

if [[ -z "$OUTPUT_NAME" || -z "$GENERATED_C" ]]; then
  echo "Usage: $0 [--target=zxn|host] <output-name> <generated-c-file>" >&2
  exit 1
fi

# Resolve absolute paths
GENERATED_C_ABS="$(cd "$(dirname "$GENERATED_C")" && pwd)/$(basename "$GENERATED_C")"
OUTPUT_ABS="$(cd "$(dirname "$OUTPUT_NAME")" 2>/dev/null && pwd)/$(basename "$OUTPUT_NAME")" || OUTPUT_ABS="$OUTPUT_NAME"

# Library paths relative to Rock root
LIB_CPU_AGNOSTIC="$ROCK_ROOT/lib/cpu_agnostic"
LIB_Z80="$ROCK_ROOT/lib/z80"
LIB_ZXN="$ROCK_ROOT/lib/zxn"

# Verify generated C file exists
if [[ ! -f "$GENERATED_C_ABS" ]]; then
  echo "Error: Generated C file not found: $GENERATED_C_ABS" >&2
  exit 1
fi

# Verify library directories exist
if [[ ! -d "$LIB_CPU_AGNOSTIC" ]]; then
  echo "Error: Library directory not found: $LIB_CPU_AGNOSTIC" >&2
  exit 1
fi

if [[ "$TARGET" == "zxn" ]]; then
  # ZX Spectrum Next target using z88dk
  echo "Compiling for ZX Spectrum Next..."

  # Verify zcc is available
  if ! command -v zcc &> /dev/null; then
    echo "Error: zcc not found. Install z88dk to compile for zxn target." >&2
    exit 1
  fi

  cd "$ROCK_ROOT/src"

  # Get basename for output
  OUTPUT_BASE="$(basename "$OUTPUT_ABS")"

  zcc +zxn -subtype=nex -startup=0 -clib=sdcc_iy \
    -pragma-include:"$LIB_ZXN/zpragma_zxn.inc" \
    -create-app \
    -o "$OUTPUT_BASE" \
    "$GENERATED_C_ABS" \
    "../lib/cpu_agnostic/alloc.c" \
    "../lib/cpu_agnostic/fundefs.c" \
    "../lib/cpu_agnostic/fundefs_internal.c" \
    "../lib/z80/asm_interop.c"

  # Move output to final location if different from ROCK_ROOT/src
  if [[ "$OUTPUT_ABS" != "$ROCK_ROOT/src"* ]]; then
    mv "$ROCK_ROOT/src/$OUTPUT_BASE" "$OUTPUT_ABS"
    # Clean up object/map files that may have been generated
    rm -f "$ROCK_ROOT/src/$OUTPUT_BASE".* 2>/dev/null || true
  fi

elif [[ "$TARGET" == "host" ]]; then
  # Host machine target using gcc
  echo "Compiling for host machine..."

  # Format the generated C code
  if command -v clang-format &> /dev/null; then
    clang-format "$GENERATED_C_ABS" -i
  fi

  gcc -Wall -g \
    -I "$ROCK_ROOT/src" \
    -o "$OUTPUT_ABS" \
    "$GENERATED_C_ABS" \
    "$LIB_CPU_AGNOSTIC/alloc.c" \
    "$LIB_CPU_AGNOSTIC/fundefs.c" \
    "$LIB_CPU_AGNOSTIC/fundefs_internal.c" \
    "$LIB_Z80/asm_interop.c"

else
  echo "Error: Unknown target '$TARGET'. Use 'host' or 'zxn'." >&2
  exit 1
fi

echo "Successfully compiled to: $OUTPUT_ABS"
