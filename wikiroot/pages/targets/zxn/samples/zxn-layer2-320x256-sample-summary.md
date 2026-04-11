---
title: ZXN Layer 2 320x256 Sample Summary
category: targets
tags: [zxn, samples, layer2, graphics, 320x256]
sources: [samples/layer2-320x256/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Layer 2 320x256 Sample Summary

Source: [samples/layer2-320x256/main.asm](../../../../raw/samples/layer2-320x256/main.asm)

This sample fills **Layer 2** in 320x256 8bpp mode. It switches Layer 2 control `$70` to 320x256 mode, configures the clip window, then fills ten 8K banks in column-major order.

## Execution Sequence

1. Enable Layer 2 through `$123B`.
2. Set the 16K start bank with `$12`.
3. Select 320x256 mode with `NEXTREG $70, %00010000`.
4. Reset the Layer 2 **Clip Window** index through `$1C`, then write X1/X2/Y1/Y2 through `$18`.
5. For each 8K bank, page it into slot 6 with `$56`.
6. Fill 32 byte-columns per bank: `E` walks Y from 0 to 255, and `D` advances X.
7. Increment the colour as columns advance.

## What It Adds

The sample makes the 320x256 memory order concrete. In this mode, one 8K bank covers 32 columns by 256 rows. The full 320-wide screen therefore needs ten 8K banks, even though the hardware reference often describes the framebuffer as five contiguous 16K banks.

## See Also

- [[targets/zxn/samples/zxn-layer2-samples]] — Layer 2 sample comparison
- [[targets/zxn/zxn-layer2]] — Layer 2 reference
