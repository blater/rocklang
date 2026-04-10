---
title: ZXN ULA Layer
category: targets
tags: [zxn, ula, display, pixels, attributes, border, shadow-screen]
sources: [zxnext_guide.md]
updated: 2026-04-10
status: current
---

# ZXN ULA Layer

The **ULA** (Uncommitted Logic Array) provides the classic ZX Spectrum 256×192 pixel display. It is the lowest-level graphics layer on the Next and is always active unless explicitly disabled via `$68`. ULA reads exclusively from 16K bank 5 (mapped to CPU addresses `$4000–$7FFF`).

The display surface is 256×192 pixels, divided into 32×24 character blocks of 8×8 pixels each. There is a 32-pixel border on all sides. The Next adds scroll offsets, ULANext palettes, and optional enhanced modes on top of the classic layout.

## Pixel Memory

6144 bytes at `$4000–$57FF`. Each byte = 8 horizontal pixels (1 bit per pixel). The layout is **non-linear** — memory is split into three 2KB "banks" of 64 character rows each:

| Memory Range | Screen Lines | Character Rows |
|---|---|---|
| `$4000–$47FF` | 0–63 | 0–7 |
| `$4800–$4FFF` | 64–127 | 8–15 |
| `$5000–$57FF` | 128–191 | 16–23 |

Within each 2KB bank, lines are interleaved. Address calculation is non-trivial. The Next adds two Z80N instructions to simplify pixel addressing (8 T-states each):

- `PIXELAD` — D=Y, E=X → HL = address of pixel byte in screen memory
- `PIXELDN` — HL points to line → update HL to same X position, one line lower
- `SETAE` — E=X → A = bitmask for the pixel within its byte

**Draw a vertical line from (32,16) to (32,50):**
```asm
LD DE, $1020    ; Y=16, X=32
PIXELAD         ; HL = pixel address
loop:
  SETAE         ; A = pixel mask
  OR (HL)       ; set bit
  LD (HL), A    ; write pixel
  INC D         ; Y++
  LD A, D
  CP 51
  RET NC
  PIXELDN       ; advance HL to next line
  JR loop
```

![Pixel memory layout](../../../raw/images/zxnext_guide_p079_f1.png)

## Attributes Memory

768 bytes at `$5800–$5AFF`. One byte per 8×8 character block (32×24 = 768 blocks).

Attribute byte layout:

| Bit | Description |
|-----|-------------|
| 7 | Flash (1=flash, 0=static) |
| 6 | Bright (1=bright palette) |
| 5–3 | Paper colour (0–7) |
| 2–0 | Ink colour (0–7) |

Classic colours: 0=Black, 1=Blue, 2=Red, 3=Magenta, 4=Green, 5=Cyan, 6=Yellow, 7=White.

![Attribute byte](../../../raw/images/zxnext_guide_p080_f1.png)

## Border

Written via port `$xxFE` bits 2–0 (same colour values 0–7 as attributes):
```asm
LD A, 1         ; blue border
OUT ($FE), A
```
Note: Layer 2 in 320×256/640×256 mode and Tilemap cover the border area.

## Shadow Screen (Double Buffering)

The ULA normally reads bank 5. Setting bit 3 of `$7FFD` switches display to bank 7 (the "shadow screen"), enabling double-buffering:
- Draw to the inactive bank
- Flip by toggling bit 3 of `$7FFD`

Since both banks fit in 8K (pixel+attribute = 6912 bytes), only the first 8K of each 16K bank needs swapping. Use `$52` (MMU slot 2) to swap banks:
```asm
NEXTREG $52, 10   ; bank 5 first half → display bank 5
NEXTREG $52, 14   ; bank 7 first half → display bank 7
```

![Shadow screen](../../../raw/images/zxnext_guide_p082_f1.png)

## Enhanced ULA Modes

The Next supports Timex Sinclair Double Buffering, Hi-Res (512×192), Hi-Colour, and LoRes modes. See the [Next Dev Wiki](https://wiki.specnext.dev/Video_Modes) for details. These modes are controlled via port `$xxFF` and register `$69`.

## Registers

**ULA Control Port Write `$xxFE`**

| Bit | Description |
|-----|-------------|
| 4 | Reserved (use 0) |
| 3 | EAR output (internal speaker) |
| 2 | MIC output (tape save) |
| 2–0 | Border colour (0–7) |

**ULA Control Port Read `$xxFE`** — see [[targets/zxn/zxn-keyboard]] (keyboard matrix input)

**ULA X Offset `$26`**
- Bits 7–0: X pixel scroll offset (0–255)

**ULA Y Offset `$27`**
- Bits 7–0: Y pixel scroll offset (0–191)

**Enhanced ULA Ink Colour Mask `$42`**
- Controls palette index split for ink/paper in ULANext mode
- See [[targets/zxn/zxn-palette]] for full description

**ULA Control `$68`**

| Bit | Description |
|-----|-------------|
| 7 | 1 = disable ULA output entirely |
| 6 | (core 3.1.1+) Blend mode: 0=ULA as blend, 1=ULA/Tilemap as blend |
| 5 | (core 3.1.4+) Cancel extended key matrix entries |
| 3 | 1 = enable ULA+ |
| 2 | 1 = enable ULA half-pixel scroll |
| 1 | Reserved (0) |
| 0 | 1 = enable stencil mode (ULA + Tilemap both enabled required) |

**Memory Paging Control `$7FFD` bit 3** — ULA shadow screen toggle (see [[targets/zxn/zxn-memory-paging]])

**Palette registers `$40`, `$41`, `$42`, `$43`, `$44`, `$4A`** — see [[targets/zxn/zxn-palette]]

**Clip Window ULA/LoRes `$1A`**
- Sequential writes: X1, X2, Y1, Y2
- Reset index via `$1C` bit 3

## See Also

- [[targets/zxn-hardware]] — layer compositing order
- [[targets/zxn/zxn-palette]] — ULA palette configuration
- [[targets/zxn/zxn-memory-paging]] — bank 5 / bank 7 shadow screen
- [[targets/zxn/zxn-tilemap]] — stencil mode (ULA + Tilemap interaction)
- [[targets/zxn/zxn-keyboard]] — reading keyboard via `$xxFE`
