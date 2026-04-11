---
title: ZXN Sample Programs
category: targets
tags: [zxn, samples, sjasmplus, nex, hardware]
sources: [samples/im1/main.asm, samples/im2/main.asm, samples/im2hw/main.asm, samples/im2safe/main.asm, samples/sprites/main.asm, samples/sprites/sprites.spr, samples/sound/main.asm, samples/tilemap/main.asm, samples/tilemap/tiles.map, samples/tilemap/tiles.spr, samples/tilemap/tiles.pal, samples/layer2-256x192/main.asm, samples/layer2-320x256/main.asm, samples/layer2-640x256/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Sample Programs

**ZXN Sample Program** pages document concrete sjasmplus assembly programs that exercise one hardware subsystem at a time. They complement the register reference pages by showing the setup order, live register values, memory placement, and `.nex` output conventions needed to make the hardware do visible work.

Use this page as the bridge between the hardware reference and implementation detail. The subsystem pages explain what the hardware can do; the sample pages explain the smallest working sequences that make it happen.

## Sample Topics

| Topic | Overview | Sample summaries | Hardware pages |
|-------|----------|------------------|----------------|
| Interrupts | [[targets/zxn/samples/zxn-interrupt-samples]] | [[targets/zxn/samples/zxn-im1-sample-summary]], [[targets/zxn/samples/zxn-im2-sample-summary]], [[targets/zxn/samples/zxn-im2hw-sample-summary]], [[targets/zxn/samples/zxn-im2safe-sample-summary]] | [[targets/zxn/zxn-interrupts]], [[targets/zxn/zxn-memory-paging]] |
| Sprites | [[targets/zxn/zxn-sprites]] | [[targets/zxn/samples/zxn-sprite-sample-summary]] | [[targets/zxn/zxn-sprites]], [[targets/zxn/zxn-dma]], [[targets/zxn/zxn-palette]] |
| Sound | [[targets/zxn/zxn-sound]] | [[targets/zxn/samples/zxn-sound-sample-summary]] | [[targets/zxn/zxn-sound]] |
| Tilemap | [[targets/zxn/zxn-tilemap]] | [[targets/zxn/samples/zxn-tilemap-sample-summary]] | [[targets/zxn/zxn-tilemap]], [[targets/zxn/zxn-palette]] |
| Layer 2 graphics | [[targets/zxn/samples/zxn-layer2-samples]] | [[targets/zxn/samples/zxn-layer2-256x192-sample-summary]], [[targets/zxn/samples/zxn-layer2-320x256-sample-summary]], [[targets/zxn/samples/zxn-layer2-640x256-sample-summary]] | [[targets/zxn/zxn-layer2]], [[targets/zxn/zxn-memory-paging]] |

## Common Assembly Shape

The current samples share a consistent wrapper:

- `DEVICE ZXSPECTRUMNEXT` enables Next paging and Z80N instructions.
- `CSPECTMAP "build/test.map"` emits a debugger map for CSpect.
- `ORG $8000` places the main routine in normal program RAM.
- Hardware setup runs before the infinite demonstration loop.
- `SAVENEX OPEN`, `SAVENEX CORE`, `SAVENEX CFG`, and `SAVENEX AUTO` produce the final **NEX file**.

This structure is useful for Rock because it separates target packaging from subsystem setup. A future Rock runtime helper can keep the `.nex` packaging in the driver/toolchain layer while exposing small routines for hardware setup and register writes.

## Implementation Themes

The samples add practical detail that is not obvious from the register tables alone:

- Order matters. Interrupt samples wrap hardware setup in `DI`/`EI` so a partially configured handler cannot fire.
- Memory placement matters. IM1 places the handler in a bank that becomes slot 0, while IM2 places vector data on alignment boundaries.
- Register programming is intentionally direct. Samples use `NEXTREG` for **Next Register** writes and 16-bit I/O ports for hardware that is not register-mirrored.
- Assets matter. Sprite and tilemap programs pair `.asm` setup with binary data loaded through `INCBIN`.
- Debuggability matters. Each sample leaves simple counters, infinite loops, or visible state changes so CSpect inspection can confirm the setup.

## See Also

- [[targets/zxn-hardware]] — subsystem overview
- [[targets/zxn-z80]] — Rock's ZXN output target
- [[syntax/embed]] — inline assembly syntax available to Rock programs
