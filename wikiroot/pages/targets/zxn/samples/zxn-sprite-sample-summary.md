---
title: ZXN Sprite Sample Summary
category: targets
tags: [zxn, samples, sprites, dma, unified-relative]
sources: [samples/sprites/main.asm, samples/sprites/sprites.spr]
updated: 2026-04-11
status: current
---

# ZXN Sprite Sample Summary

Source: [samples/sprites/main.asm](../../../../raw/samples/sprites/main.asm)  
Asset: [samples/sprites/sprites.spr](../../../../raw/samples/sprites/sprites.spr)

This sample demonstrates hardware **Sprite** setup, DMA upload of **Sprite Pattern** data, single sprites, and a four-sprite **Unified Relative Sprite** group.

## Execution Sequence

1. Start at `$8000`.
2. Load pattern data from `sprites.spr` into FPGA sprite memory with the `loadSprites` routine.
3. Enable sprites with `NEXTREG $15, %01000001`, choosing sprite-on-top `S L U` priority.
4. Select sprite slots through `$34`.
5. Write position, transform, visibility, optional byte-4, and pattern values through `$35`-`$39`.
6. Build a 2x2 unified relative group using sprite 2 as the anchor and sprites 3-5 as relative sprites.
7. After delays, rewrite the anchor attributes to move, rotate, mirror, and scale the whole group.

## Asset Shape

`sprites.spr` is a 16,384-byte binary sprite sheet. The caller asks to copy `16*16*5 = 1,280` bytes, enough for five 8-bit 16x16 patterns. The rest of the file remains available for additional patterns.

## DMA Pattern Upload

The sample uses the standard memory-to-I/O DMA shape:

- set the starting sprite pattern slot by writing `A` to port `$303B`;
- copy the sprite-sheet address into the DMA program's Port A address;
- copy the requested byte count into the DMA program's length field;
- use fixed I/O Port B `$005B`, the sprite pattern upload port;
- upload and run the DMA program through port `$6B` with `OTIR`.

> **TODO:** Confirm whether `loadSprites` should preserve the caller's `BC` byte count before loading `$303B` into `BC`. As written, `.dmaLength` receives `$303B` instead of the requested `16*16*5`, so the sample appears to upload more pattern data than the comment describes.

## Attribute Pattern

The first two sprites are simple single-sprite cases using pattern 0:

- `$34` selects sprite 0 or 1.
- `$35` and `$36` set X/Y.
- `$37` sets palette offset and transform bits.
- `$38` sets visible and pattern 0.

The group starts at sprite 2. The anchor sets byte 4 with `$79, %00100000`, marking unified relatives. Each relative then writes an offset from the anchor and ends with `$79, %01000000`.

## Rock Runtime Implications

Sprite support needs two layers of helper:

- a bulk upload helper for pattern data, likely backed by [[targets/zxn/zxn-dma]];
- attribute writers for simple sprites and anchor/relative groups, backed by `$34`-`$39` or `$75`-`$79`.

The sample's direct register sequence is a good minimum for inline assembly examples, but reusable Rock helpers should avoid exposing raw bit packing at every call site.

## See Also

- [[targets/zxn/zxn-sprites]] — sprite hardware reference
- [[targets/zxn/zxn-dma]] — DMA programming model
- [[targets/zxn/zxn-sample-programs]] — sample-program hub
