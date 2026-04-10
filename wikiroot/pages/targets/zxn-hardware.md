---
title: ZXN Hardware Overview
category: targets
tags: [zxn, hardware, overview, graphics-layers, priority, spectrum-next]
sources: [zxnext_guide.md]
updated: 2026-04-10
status: current
---

# ZXN Hardware Overview

The ZX Spectrum Next is an FPGA-based reimplementation and extension of the ZX Spectrum. For a Rock implementer, it provides a rich set of hardware subsystems accessible via Z80 I/O ports and **Next registers** (`$243B`/`$253B` or the `NEXTREG` instruction).

This page is the entry point to the hardware reference. Each subsystem has its own detail page.

## Hardware Subsystems

| Subsystem | What it does |
|-----------|-------------|
| [[targets/zxn/zxn-ports-registers]] | Full I/O port map and Next register index |
| [[targets/zxn/zxn-memory-paging]] | 8K/16K bank paging, MMU slots, legacy 128K modes |
| [[targets/zxn/zxn-dma]] | Bulk memory/IO transfer, fixed-time audio streaming |
| [[targets/zxn/zxn-palette]] | 256-entry colour palettes (8-bit RRRGGGBB or 9-bit), shared by all layers |
| [[targets/zxn/zxn-ula]] | Classic 256×192 1-bit pixel display with 8×8 colour attributes |
| [[targets/zxn/zxn-layer2]] | Full-colour framebuffer: 256×192, 320×256 or 640×256 |
| [[targets/zxn/zxn-tilemap]] | 8×8 tile block display: 40×32 or 80×32 tiles |
| [[targets/zxn/zxn-sprites]] | 128 hardware sprites, 16×16 pixels, anchor/relative groups |
| [[targets/zxn/zxn-copper]] | Raster co-processor: changes registers at specific scanline positions |
| [[targets/zxn/zxn-sound]] | 3× AY-3-8912 (9 channels), DAC, Turbo Sound |
| [[targets/zxn/zxn-keyboard]] | 8×5 key matrix + 10 extended keys |
| [[targets/zxn/zxn-interrupts]] | IM1 / IM2 / Hardware IM2 with 14 vectored sources |

## Graphics Layer Stack

The Next composites four graphics layers. Rendering order (bottom to top) is configurable via `$15` (Sprite and Layers System), but the typical default is:

```
┌─────────────────────────────────┐
│  Sprites (layer S)              │  ← 128 hw sprites, 16×16px
├─────────────────────────────────┤
│  Layer 2 (layer L)              │  ← full-colour framebuffer
├─────────────────────────────────┤
│  Tilemap (layer T)              │  ← 8×8 tile blocks
├─────────────────────────────────┤
│  ULA (layer U)                  │  ← classic 256×192 + attributes
├─────────────────────────────────┤
│  Border / Fallback ($4A)        │  ← shown where all layers transparent
└─────────────────────────────────┘
```

The standard priority modes (`$15` bits 4–2):
- `000` = S L U (Sprites top, Layer 2 middle, ULA bottom)
- Others = blending modes (core 3.1.1+ for full support)

Tilemap can appear above or below ULA (per-tile via attribute bit 0, or global via `$6B` bit 0).

Layer 2 has a **per-colour priority flag** (bit 7 of 9-bit palette second byte) that forces individual colours to the top of all layers.

Each layer has an **independent transparent colour**:
- ULA/Layer 2/LoRes: Global Transparency `$14`
- Sprites: `$4B`
- Tilemap: `$4C` (4-bit index)
- All transparent → fallback colour `$4A`

## Port Access Patterns

### Writing to Next Registers

Always prefer `NEXTREG` (20–24 T-states) over the `$243B`/`$253B` port method (52–58 T-states):

```asm
NEXTREG $16, 5     ; write 5 to Layer 2 X offset: 24 T-states (immediate)
NEXTREG $16, A     ; write A to Layer 2 X offset: 20 T-states
```

### Reading from Next Registers

No `NEXTREG` read instruction exists; always use ports:
```asm
LD BC, $243B
LD A, $16          ; register to read
OUT (C), A
INC B              ; BC = $253B
IN A, (C)          ; A = current value
```

### Writing to Spectrum Ports (ULA, sound, memory paging)

```asm
LD A, value
OUT ($FE), A       ; 8-bit port (A = high byte): 18 T-states
; or:
LD BC, $FFFD
OUT (C), A         ; 16-bit port: 29 T-states
```

## Key Architectural Points for Rock Implementers

1. **NEXTREG is the primary write mechanism** — use it for all Next register writes. It's the fastest and most readable approach.
2. **Layer 2 requires bank paging** — the framebuffer is too large for Z80's address space. Use MMU slot 6 (`$56`) to access 8K at a time (see [[targets/zxn/zxn-layer2]]).
3. **Sprite patterns live in FPGA RAM** — upload once via DMA or port `$xx5B`; they persist until overwritten (see [[targets/zxn/zxn-sprites]], [[targets/zxn/zxn-dma]]).
4. **Palettes are shared infrastructure** — all layers read from the same hardware palette space. Be careful when switching palette edit targets via `$43` to not accidentally change the active display palette.
5. **The Copper is free CPU time** — offload per-scanline register changes to the Copper instead of using a line interrupt ISR (see [[targets/zxn/zxn-copper]]).
6. **DMA blocks the CPU in continuous mode** — for large transfers, consider burst mode with a prescalar, or schedule DMA during vblank (see [[targets/zxn/zxn-dma]]).

## See Also

- [[targets/zxn-z80]] — ZXN compilation target (toolchain, `rock --target=zxn`)
- [[targets/zxn/zxn-ports-registers]] — complete port and register index
