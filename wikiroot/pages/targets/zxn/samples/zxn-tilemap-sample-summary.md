---
title: ZXN Tilemap Sample Summary
category: targets
tags: [zxn, samples, tilemap, palette, assets]
sources: [samples/tilemap/main.asm, samples/tilemap/tiles.map, samples/tilemap/tiles.spr, samples/tilemap/tiles.pal]
updated: 2026-04-11
status: current
---

# ZXN Tilemap Sample Summary

Source: [samples/tilemap/main.asm](../../../../raw/samples/tilemap/main.asm)  
Assets: [tiles.map](../../../../raw/samples/tilemap/tiles.map), [tiles.spr](../../../../raw/samples/tilemap/tiles.spr), [tiles.pal](../../../../raw/samples/tilemap/tiles.pal)

This sample demonstrates **Tilemap** setup in 40x32 1-byte mode, using externally prepared **Tilemap Entry**, **Tile Definition**, and palette assets. It also shows the X-offset register used as a simple screen-shake effect.

## Asset Shape

| Asset | Size | Meaning |
|-------|------|---------|
| `tiles.map` | 1,280 bytes | 40x32 tilemap entries, one byte per tile. |
| `tiles.spr` | 192 bytes | 4-bit tile-definition data trimmed from a sprite-editor export. |
| `tiles.pal` | 16 bytes | 16 RRRGGGBB palette entries, one byte per colour. |

The source notes that the files were produced with the Remy Sharp ZX sprite editor, then manually trimmed for the exact format the sample expects.

## Execution Sequence

1. Start at `$8000`.
2. Enable tilemap mode with `NEXTREG $6B, %10100001`.
3. Set the default tile attribute with `NEXTREG $6C, %00000000`.
4. Point `$6E` at `$6000` and `$6F` at `$6600`, both expressed as offsets from bank 5 `$4000`.
5. Select the first tilemap palette through `$43` and index 0 through `$40`.
6. Copy 16 palette entries to `$41`.
7. Copy tile definitions to `$6600` with `LDIR`.
8. Copy the 40x32 map to `$6000` with `LDIR`.
9. Toggle Tilemap Offset X LSB `$30` between 0 and 1 to simulate shake.

## Memory Layout

The sample keeps all tilemap data in 16K bank 5, after the ULA screen and attributes:

| Data | Address | Offset register value |
|------|---------|-----------------------|
| Tilemap entries | `$6000` | `$20` for `$6E` |
| Tile definitions | `$6600` | `$26` for `$6F` |

This matches the recommended layout on [[targets/zxn/zxn-tilemap]] and avoids overlapping ULA pixel/attribute memory.

## Data Copy Routines

`copyTileDefinitions` and `copyTileMap` are plain `LDIR` copies into the addresses selected by `$6E` and `$6F`. `copyPalette` writes one byte per colour to `$41`, relying on `$43` auto-increment setup.

The `copyPalette256` variant demonstrates the `DJNZ` idiom where `B = 0` means 256 iterations, although the sample calls the 16-colour version.

## Rock Runtime Implications

Tilemap support needs asset-aware helpers as much as register wrappers:

- loaders for map, tile, and palette byte arrays;
- constants for bank 5 tilemap base offsets;
- a safe palette writer for `$40`/`$41`/`$43`;
- scroll/shake helpers for `$2F`, `$30`, and `$31`.

## See Also

- [[targets/zxn/zxn-tilemap]] — tilemap reference
- [[targets/zxn/zxn-palette]] — palette upload registers
- [[targets/zxn/zxn-sample-programs]] — sample-program hub
