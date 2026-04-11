---
title: ZXN Layer 2 256x192 Sample Summary
category: targets
tags: [zxn, samples, layer2, graphics, 256x192]
sources: [samples/layer2-256x192/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Layer 2 256x192 Sample Summary

Source: [samples/layer2-256x192/main.asm](../../../../raw/samples/layer2-256x192/main.asm)

This sample fills **Layer 2** in 256x192 8bpp mode by writing an X-coordinate gradient across six 8K banks.

## Execution Sequence

1. Enable Layer 2 through `$123B`.
2. Set the 16K start bank with `NEXTREG $12, START_16K_BANK`.
3. Loop Y from 0 to 191.
4. Compute the 8K bank with the top three bits of Y, equivalent to `START_8K_BANK + (Y >> 5)`.
5. Page that bank into slot 6 with `$56`.
6. Convert `(X,Y)` into `$C000 + ((Y & 31) << 8) + X`.
7. Write the X value as the colour byte.

## What It Adds

The sample is the clearest practical reference for 256x192 drawing through an 8K slot. A 16K Layer 2 bank contains 64 rows, but slot 6 exposes only 32 rows at a time, so code using `$56` must page six 8K banks for a full 256x192 screen.

## See Also

- [[targets/zxn/samples/zxn-layer2-samples]] — Layer 2 sample comparison
- [[targets/zxn/zxn-layer2]] — Layer 2 reference
