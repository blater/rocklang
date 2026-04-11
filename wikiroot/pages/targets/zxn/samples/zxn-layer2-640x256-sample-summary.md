---
title: ZXN Layer 2 640x256 Sample Summary
category: targets
tags: [zxn, samples, layer2, graphics, 640x256]
sources: [samples/layer2-640x256/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Layer 2 640x256 Sample Summary

Source: [samples/layer2-640x256/main.asm](../../../../raw/samples/layer2-640x256/main.asm)

This sample fills **Layer 2** in 640x256 4bpp mode. The code is structurally close to the 320x256 sample, but `$70` selects 4bpp mode and each byte represents two horizontal pixels.

## Execution Sequence

1. Enable Layer 2 through `$123B`.
2. Set the 16K start bank with `$12`.
3. Select 640x256 mode with `NEXTREG $70, %00100000`.
4. Reset and write the Layer 2 **Clip Window**, using `RESOLUTION_X / 4 - 1` for X2.
5. Page ten 8K banks through `$56`.
6. Fill 32 byte-columns per bank, which corresponds to 64 screen pixels per bank because each byte stores two 4bpp pixels.
7. Increment the colour byte as byte-columns advance.

## What It Adds

The sample highlights the key 640x256 difference: the framebuffer still occupies 80KB, but each byte packs two pixels. Clip-window X coordinates are quartered, and addressing uses byte-columns rather than pixel columns.

## See Also

- [[targets/zxn/samples/zxn-layer2-samples]] — Layer 2 sample comparison
- [[targets/zxn/zxn-layer2]] — Layer 2 reference
