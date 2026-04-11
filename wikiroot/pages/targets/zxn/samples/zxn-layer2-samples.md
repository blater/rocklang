---
title: ZXN Layer 2 Samples
category: targets
tags: [zxn, samples, layer2, graphics, banking]
sources: [samples/layer2-256x192/main.asm, samples/layer2-320x256/main.asm, samples/layer2-640x256/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Layer 2 Samples

The Layer 2 samples compare the three **Layer 2** modes from the point of view of code that draws through **MMU Slot** 6 (`$C000-$DFFF`). They show the difference between the 16K start bank selected by `$12` and the 8K banks actually paged with `$56`.

## Source Map

| Source | Summary | Mode | Drawing order |
|--------|---------|------|---------------|
| [samples/layer2-256x192/main.asm](../../../../raw/samples/layer2-256x192/main.asm) | [[targets/zxn/samples/zxn-layer2-256x192-sample-summary]] | 256x192, 8bpp | Rows, 32 lines per 8K bank |
| [samples/layer2-320x256/main.asm](../../../../raw/samples/layer2-320x256/main.asm) | [[targets/zxn/samples/zxn-layer2-320x256-sample-summary]] | 320x256, 8bpp | Columns, 32 byte-columns per 8K bank |
| [samples/layer2-640x256/main.asm](../../../../raw/samples/layer2-640x256/main.asm) | [[targets/zxn/samples/zxn-layer2-640x256-sample-summary]] | 640x256, 4bpp | Byte-columns, 64 pixels per 8K bank |

## Shared Setup

All three samples:

- start at `$8000`;
- enable Layer 2 with port `$123B`, value `2`;
- set the Layer 2 16K start bank through `NEXTREG $12, 9`;
- page individual 8K banks into slot 6 with `NEXTREG $56, bank`;
- fill visible memory at `$C000`;
- end in an infinite loop so the result remains visible.

## Addressing Comparison

| Mode | Bytes | 8K banks | Bank selector | Offset within slot 6 |
|------|-------|----------|---------------|----------------------|
| 256x192 8bpp | 49,152 | 6 | `START_8K_BANK + (Y >> 5)` | `((Y & 31) << 8) | X` |
| 320x256 8bpp | 81,920 | 10 | `START_8K_BANK + (X / 32)` | `((X & 31) << 8) | Y` |
| 640x256 4bpp | 81,920 | 10 | `START_8K_BANK + ((X / 2) / 32)` | `(((X / 2) & 31) << 8) | Y` |

The 320x256 and 640x256 modes are column-major from the sample's perspective: the low byte tracks Y, and the high byte advances through X byte-columns.

## Clip Windows

The wide modes configure the **Clip Window** before drawing:

- 320x256 writes X2 as `RESOLUTION_X / 2 - 1`, because Layer 2 clip X values are halved in that mode.
- 640x256 writes X2 as `RESOLUTION_X / 4 - 1`, because Layer 2 clip X values are quartered in that mode.

The 256x192 sample uses the default clip and does not write `$18`.

## See Also

- [[targets/zxn/zxn-layer2]] — Layer 2 reference
- [[targets/zxn/zxn-memory-paging]] — 8K bank paging
- [[targets/zxn/zxn-sample-programs]] — sample-program hub
