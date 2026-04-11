---
title: ZXN IM2 Sample Summary
category: targets
tags: [zxn, samples, interrupts, im2]
sources: [samples/im2/main.asm]
updated: 2026-04-11
status: current
---

# ZXN IM2 Sample Summary

Source: [samples/im2/main.asm](../../../../raw/samples/im2/main.asm)

This sample demonstrates legacy **IM2** with a conventional 256-byte aligned table. The setup routine writes 128 16-bit entries, all pointing at the same `InterruptHandler`, then loads the high byte of the table address into `I`.

## Execution Sequence

1. Start at `$8000` and set CPU speed with `NEXTREG $07, 3`.
2. Disable interrupts with `DI`.
3. Call `SetupInterruptVectors`.
4. Fill `InterruptVectorTable` with 128 copies of `InterruptHandler`.
5. Load `I = InterruptVectorTable >> 8`.
6. Select `IM 2`, then `EI`.
7. The handler increments `counter` and returns with `RETI`.

## What It Adds

The sample makes the legacy IM2 table shape explicit: 128 words on a 256-byte boundary. It is the simplest readable version of IM2 setup and a useful stepping stone before the safer 257-byte variant.

## Constraints

This version does not handle every possible data-bus LSB value safely. The [[targets/zxn/samples/zxn-im2safe-sample-summary]] variant addresses that by filling 257 bytes with a repeated handler-address byte and placing the handler at an address where both bytes match.

## See Also

- [[targets/zxn/samples/zxn-interrupt-samples]] — comparison across interrupt samples
- [[targets/zxn/zxn-interrupts]] — legacy IM2 reference
