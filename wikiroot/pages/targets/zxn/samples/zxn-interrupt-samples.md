---
title: ZXN Interrupt Samples
category: targets
tags: [zxn, samples, interrupts, im1, im2, hardware-im2]
sources: [samples/im1/main.asm, samples/im2/main.asm, samples/im2hw/main.asm, samples/im2safe/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Interrupt Samples

The interrupt samples compare **IM1**, **IM2**, and **Hardware IM2** setup on the ZX Spectrum Next. All four programs increment a counter from an **interrupt handler** while the main loop periodically reads the counter for debugger inspection.

## Source Map

| Source | Summary | Main idea |
|--------|---------|-----------|
| [samples/im1/main.asm](../../../../raw/samples/im1/main.asm) | [[targets/zxn/samples/zxn-im1-sample-summary]] | Page a custom bank into slot 0 so the frame interrupt jumps to `$0038`. |
| [samples/im2/main.asm](../../../../raw/samples/im2/main.asm) | [[targets/zxn/samples/zxn-im2-sample-summary]] | Fill a 256-byte table with 128 copies of the handler address. |
| [samples/im2safe/main.asm](../../../../raw/samples/im2safe/main.asm) | [[targets/zxn/samples/zxn-im2safe-sample-summary]] | Use a 257-byte bus-safe table and a handler whose high and low bytes match. |
| [samples/im2hw/main.asm](../../../../raw/samples/im2hw/main.asm) | [[targets/zxn/samples/zxn-im2hw-sample-summary]] | Use the Next's 32-byte hardware vector table and per-source enable/status registers. |

## Shared Setup Pattern

The legacy and hardware IM2 variants follow the same high-level sequence:

1. Set CPU speed with `NEXTREG $07, 3` for a visible difference in debugger timing.
2. Execute `DI` before touching vectors, `I`, or Next interrupt registers.
3. Build or select the **interrupt vector table**.
4. Load `I` with the vector table's high byte when using IM2.
5. Select `IM 1` or `IM 2`.
6. Execute `EI` only after all dependent state is valid.
7. Keep the main program in a simple loop so the handler activity is isolated.

## Choosing an Interrupt Style

| Mode | Best fit | Setup cost | Risk |
|------|----------|------------|------|
| IM1 | Minimal frame interrupt demonstration or ROM-replacement experiments | Low: map a bank into slot 0 and place code at `$0038` | Replaces ROM in slot 0; handler code must be present at the reset vector area. |
| IM2 | Basic vectored interrupt handler | Medium: 256-byte aligned table, 128 address entries | Depends on the data-bus LSB; not the safest form for reusable runtime code. |
| Safe IM2 | Robust legacy IM2 handler | Medium: 257-byte table and carefully placed handler | Handler placement is constrained to an address like `$F0F0`. |
| Hardware IM2 | Multiple interrupt sources or Next-specific handlers | Higher: `$C0`, `$C4`, `$C5`, `$C6`, table alignment, status clearing | More registers to coordinate, but per-source routing is explicit. |

## Rock Runtime Implications

These samples suggest that Rock should treat interrupts as target-runtime helpers rather than compiler syntax at first. The low-level operations needed are already expressible through inline assembly, but reusable helpers would need to own:

- a stable code/data placement strategy for vector tables and handlers;
- a critical-section convention around `DI`/`EI`;
- register preservation policy inside handlers;
- optional wrappers for `$C0`/`$C4`/`$C5`/`$C6` when using Hardware IM2.

## Review Notes

The IM1, IM2, and IM2-safe examples intentionally keep handlers small and do not preserve the full register set. The Hardware IM2 sample does preserve alternate and main register sets with `EX AF, AF'` and `EXX`, making it the better pattern for reusable code.

## See Also

- [[targets/zxn/zxn-interrupts]] — interrupt modes and register reference
- [[targets/zxn/zxn-memory-paging]] — MMU slot paging used by the IM1 example
- [[targets/zxn/zxn-sample-programs]] — sample-program hub
