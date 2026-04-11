---
title: ZXN Copper Sample Summary
category: targets
tags: [zxn, samples, copper, layer2, dma, palette]
sources: [samples/copper/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Copper Sample Summary

Source: [samples/copper/main.asm](../../../../raw/samples/copper/main.asm)

This sample demonstrates **Copper** setup, **Copper List** upload, palette changes at raster positions, live patching of Copper instructions, and Layer 2 clip-window manipulation.

## Execution Sequence

1. Enable Layer 2 and set its 16K start bank to 9.
2. Clear Layer 2 memory by paging 8K banks into slot 6 and using DMA to copy one zero byte across each bank.
3. Upload `CopperList` either byte-by-byte through `$63` or with DMA to the `$253B` NextReg data port after selecting register `$63` through `$243B`.
4. Start the Copper in mode `%11`, resetting the Copper PC on each vertical blank.
5. Animate between Y positions 10 and 93.
6. Patch the WAIT instruction for the red palette band and the MOVE instruction for Layer 2 clip-window left position.

## Copper Program Shape

The sample defines macros for the core instruction forms:

- `COPPER_WAIT hor, ver` encodes a raster wait.
- `COPPER_MOVE reg, value` writes one Next register.
- `COPPER_NOOP` emits a no-op MOVE.
- `COPPER_HALT` emits `$FFFF`.
- `COPPER_9BIT red, green, blue` writes a 9-bit palette entry through two `$44` moves.

The list selects the first Layer 2 palette, targets palette index 0, then changes that entry at several raster lines. It also writes `$1C` and `$18` to reset and update the Layer 2 clip window mid-frame.

## Upload Paths

`InitializeCopper` uploads bytes directly through `$63`, the 16-bit Copper data write register. `$63` waits for both instruction bytes before committing, so it is safer than `$60` for full-program upload.

`InitializeCopperListDMA` demonstrates the DMA route. It first selects Next register `$63` by writing to `$243B`, then streams the Copper list to `$253B` with Port B fixed as I/O.

## Live Patching

The animation routines deliberately use `$60`, not `$63`, when changing a single byte of an existing instruction:

- `UpdateCopperRedWait` seeks to the low byte of the red WAIT instruction and writes the new Y value.
- `UpdateCopperWindowLeftPos` seeks to the MOVE value byte for `$18` and writes the new clip-window left position.

This distinction is the main practical lesson of the sample: use `$63` for whole instructions and `$60` for one-byte patches after manually setting the Copper PC.

> **TODO:** Confirm the Layer 2 clear-loop bank count. `L2_START_8K_BANK` is 18 and `L2_END_8K_BANK` is defined as `START + 6`, then the loop exits after comparing with `L2_END_8K_BANK + 1`; that appears to clear banks 18 through 24 inclusive, seven 8K banks, while 256x192 Layer 2 needs six.

## Rock Runtime Implications

Useful Copper support can be layered:

- macros or helpers for WAIT/MOVE/HALT encoding;
- a safe upload helper using `$63`;
- a live-patch helper that seeks with `$61`/`$62` and writes one byte through `$60`;
- optional DMA upload support for larger Copper lists.

## See Also

- [[targets/zxn/zxn-copper]] — Copper reference
- [[targets/zxn/zxn-layer2]] — Layer 2 clip window and memory setup
- [[targets/zxn/zxn-dma]] — DMA programming model
- [[targets/zxn/zxn-sample-programs]] — sample-program hub
