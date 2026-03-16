#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROCK_ROOT="$(dirname "$SCRIPT_DIR")"

TARGET="${1#--target=}"
OUTPUT="$2"
[[ "$OUTPUT" != /* ]] && OUTPUT="$PWD/$OUTPUT"
GENERATED_C="$(cd "$(dirname "$3")" && pwd)/$(basename "$3")"

LIBS=(
  "$ROCK_ROOT/lib/cpu_agnostic/alloc.c"
  "$ROCK_ROOT/lib/cpu_agnostic/fundefs.c"
  "$ROCK_ROOT/lib/cpu_agnostic/fundefs_internal.c"
  "$ROCK_ROOT/lib/z80/asm_interop.c"
)

if [[ "$TARGET" == "zxn" ]]; then
  # Copy headers to current directory (SDCC doesn't handle absolute paths in #include)
  cp "$ROCK_ROOT/src"/*.h .

  zcc +zxn -subtype=nex -startup=1 -clib=sdcc_iy \
    -pragma-include:"$ROCK_ROOT/lib/zxn/zpragma_zxn.inc" \
    -create-app \
    -o "$OUTPUT" \
    "$GENERATED_C" \
    "${LIBS[@]}"

  # Clean up headers
  rm -f alloc.h fundefs.h fundefs_internal.h typedefs.h
else
  gcc -Wall -g \
    -I "$ROCK_ROOT/src" \
    -o "$OUTPUT" \
    "$GENERATED_C" \
    "${LIBS[@]}"
fi
