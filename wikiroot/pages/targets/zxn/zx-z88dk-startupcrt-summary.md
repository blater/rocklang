---
title: Z88DK Startup CRT Notes
category: targets
tags: [zxn, z88dk, crt, startup, stdout, control-codes]
sources: [zx-z88dk-startupcrt.md]
updated: 2026-04-11
status: current
---

# Z88DK Startup CRT Notes

This source explains how Z88DK's new C library composes its **startup CRT** from device drivers. The startup selection decides which stdin/stdout/stderr drivers exist before `main()` runs, and therefore affects program size, text output capability, and whether terminal control codes work.

The note is written for the `+zx` target, but the startup model is relevant to Rock's `+zxn` path because `rock --target=zxn` invokes Z88DK's `zcc` frontend and selects a startup profile.

## Startup Values Mentioned

| Startup | Output driver behaviour |
|---------|-------------------------|
| `-startup=0` | 32x24 output, no control-code handling |
| `-startup=1` | 32x24 output with control-code handling |
| `-startup=4` | 64x24 fixed-width 4x8 font, no control-code handling |
| `-startup=5` | 64x24 fixed-width 4x8 font with control-code handling |
| `-startup=8` | FZX proportional font, no control-code handling |
| `-startup=9` | FZX proportional font with control-code handling |
| `-startup=31` | No stdin, stdout, or stderr; minimal-size build |

Adding 32 to the startup value selects the corresponding Interface 2 cartridge variant.

## Why It Matters for Rock

Rock's current ZXN command uses `-startup=1`, which selects the 32x24 output driver with control-code support. This is a pragmatic default for test output and simple text programs.

For size-sensitive ZXN programs, `-startup=31` is attractive because it omits standard streams, but code that relies on `print`, `printf`, file-like I/O, or terminal control codes needs a different output path first.

## Control Codes and `printf`

The source distinguishes drivers with and without control-code handling. Control-code-capable startup profiles cost more program space, but allow standard-library text output to do more work.

Z88DK also supports `printf` configuration pragmas such as `CLIB_OPT_PRINTF` to remove unused converters. This is relevant to Rock because `print` and some assertions eventually route through C runtime output, and ZXN memory pressure is real.

## Custom CRT Direction

Z88DK can use custom CRTs that compose drivers differently, including windowed terminal drivers. Rock does not expose this yet. Future target-specific runtime work could make startup profile and output-driver selection explicit in the Rock driver or a ZXN runtime configuration file.

See [[targets/zxn-z80]] for the current command line and [[syntax/builtins-and-io]] for Rock built-ins affected by target I/O support.
