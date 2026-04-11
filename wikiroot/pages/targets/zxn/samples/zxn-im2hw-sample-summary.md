---
title: ZXN Hardware IM2 Sample Summary
category: targets
tags: [zxn, samples, interrupts, hardware-im2]
sources: [samples/im2hw/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Hardware IM2 Sample Summary

Source: [samples/im2hw/main.asm](../../../../raw/samples/im2hw/main.asm)

This sample demonstrates **Hardware IM2**, the Next-specific vectored interrupt controller. Instead of a 128-entry legacy table, it uses a 32-byte aligned table containing one 16-bit handler address per hardware interrupt source.

## Execution Sequence

1. Start at `$8000` and set CPU speed with `NEXTREG $07, 3`.
2. Disable interrupts with `DI`.
3. Write `NEXTREG $C0, (InterruptVectorTable & %11100000) | %00000001` to set the vector LSB high bits and enable hardware IM2 mode.
4. Enable expansion-bus INT and ULA interrupts with `NEXTREG $C4, %10000001`.
5. Disable CTC and UART interrupts through `$C5` and `$C6`.
6. Load `I = InterruptVectorTable >> 8`.
7. Select `IM 2`, then `EI`.
8. Route source 0 to a line handler, source 11 to a ULA handler, and other sources to shared handlers.

## What It Adds

The sample shows the exact relationship between table alignment, `$C0` bits 7-5, and the vector table labels. It also demonstrates source-specific status clearing through `$C8` before falling through to a shared handler.

## Handler Pattern

The handler uses `EX AF, AF'` and `EXX` before touching registers, then restores them before `EI` and `RETI`. This is the safest register-preservation pattern among the interrupt samples and should be treated as the baseline for reusable code.

## Open Detail

The sample includes two padding vectors after the 14 documented sources. That matches the 32-byte table size described by the hardware model, but only the first 14 entries correspond to named interrupt sources.

## See Also

- [[targets/zxn/samples/zxn-interrupt-samples]] — comparison across interrupt samples
- [[targets/zxn/zxn-interrupts]] — Hardware IM2 register reference
- [[targets/zxn/zxn-dma]] — DMA interrupt interaction
