---
title: ZXN IM1 Sample Summary
category: targets
tags: [zxn, samples, interrupts, im1, memory-paging]
sources: [samples/im1/main.asm]
updated: 2026-04-11
status: current
---

# ZXN IM1 Sample Summary

Source: [samples/im1/main.asm](../../../../raw/samples/im1/main.asm)

This sample demonstrates **IM1** by replacing the ROM-visible interrupt vector area with an 8K RAM bank. The handler is assembled into `INTERRUPT_PAGE = 28` at `ORG $C038`, then the program maps that same 8K bank into **MMU Slot** 0 with `NEXTREG $50, INTERRUPT_PAGE`. Once mapped at `$0000-$1FFF`, the handler appears at runtime address `$0038`.

## Execution Sequence

1. Start the main program at `$8000`.
2. Set the CPU speed with `NEXTREG $07, 3`.
3. Disable interrupts with `DI`.
4. Page bank 28 into slot 0 using `NEXTREG $50, 28`.
5. Select `IM 1`.
6. Re-enable interrupts with `EI`.
7. Loop in the foreground and read `counter`.
8. On each frame interrupt, run the handler at `$0038`, increment `counter`, then `RETI`.

## What It Adds

The existing interrupt page explains that IM1 jumps to `$0038`; this sample shows how sjasmplus source placement and Next paging combine to put custom code there without storing it at low memory during assembly.

## Constraints

Paging RAM into slot 0 hides ROM. That is useful for a controlled demo but is a heavy-handed runtime strategy. A general Rock runtime would need to account for ROM calls, reset vectors, and any other code expecting the standard low-memory layout.

## See Also

- [[targets/zxn/samples/zxn-interrupt-samples]] — comparison across interrupt samples
- [[targets/zxn/zxn-interrupts]] — IM1 reference
- [[targets/zxn/zxn-memory-paging]] — bank and slot mapping
