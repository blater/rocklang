---
title: ZXN IM2 Safe Sample Summary
category: targets
tags: [zxn, samples, interrupts, im2]
sources: [samples/im2safe/main.asm]
updated: 2026-04-11
status: current
---

# ZXN IM2 Safe Sample Summary

Source: [samples/im2safe/main.asm](../../../../raw/samples/im2safe/main.asm)

This sample demonstrates a bus-safe legacy **IM2** setup. It places the handler at `ORG $F0F0`, fills 257 bytes with `$F0`, and points `I` at the vector table high byte. Any data-bus LSB from `$00` through `$FF` therefore resolves to a two-byte vector value of `$F0F0`.

## Execution Sequence

1. Start at `$8000` and set CPU speed with `NEXTREG $07, 3`.
2. Disable interrupts with `DI`.
3. Copy the high byte of `InterruptHandler` into the first vector byte.
4. Use `LDIR` to fill the remaining 256 bytes.
5. Load `I = InterruptVectorTable >> 8`.
6. Select `IM 2`, then `EI`.
7. The handler at `$F0F0` increments `counter` and returns with `RETI`.

## What It Adds

The sample captures the defensive IM2 idiom that the plain IM2 example omits: allocate one extra vector byte and force the target address to be byte-symmetric. This is the stronger pattern for runtime code that cannot assume a stable data-bus value.

## Constraints

The handler address is part of the safety argument. Moving the handler without keeping the high and low bytes identical invalidates the one-byte table fill technique.

## See Also

- [[targets/zxn/samples/zxn-interrupt-samples]] — comparison across interrupt samples
- [[targets/zxn/samples/zxn-im2-sample-summary]] — simpler legacy IM2 version
- [[targets/zxn/zxn-interrupts]] — legacy IM2 reference
