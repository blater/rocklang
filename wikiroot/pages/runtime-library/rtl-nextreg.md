---
title: RTL Component — nextreg (ZXN Next registers)
category: generator
tags: [rtl, nextreg, next-register, mmu, cpu-speed, zxn, host]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: nextreg

Direct access to the ZX Spectrum Next register file, CPU speed control, and MMU slot mapping.

## Rock API

```rock
next_reg_set(to_byte(7),  to_byte(3));       // write reg $07 = 3 (28 MHz)
byte v := next_reg_get(to_byte(7));          // read reg $07

cpu_speed_set(to_byte(2));                   // 14 MHz
byte s := cpu_speed_get();                   // → 2

mmu_set(to_byte(6), to_byte(20));            // slot 6 → 8K page 20
```

| Call | Return | Notes |
|------|--------|-------|
| `next_reg_set(reg, val)` | `void` | Write `val` to Next register `reg` |
| `next_reg_get(reg)` | `byte` | Read Next register `reg` |
| `cpu_speed_set(speed)` | `void` | Set turbo mode: 0=3.5 MHz, 1=7 MHz, 2=14 MHz, 3=28 MHz |
| `cpu_speed_get()` | `byte` | Read current CPU speed code (low 2 bits of reg `$07`) |
| `mmu_set(slot, page)` | `void` | Map 8K page `page` into MMU slot `slot` (0–7); `slot > 7` is a silent no-op |

## Implementation

**Source:** `src/lib/nextreg.{h,c}`

### ZXN path

Next registers are accessed via I/O ports:
- `$243B` — register select
- `$253B` — read/write

```asm
ld  a, reg
ld  bc, #$243B
out (c), a          ; select register
ld  a, val
ld  bc, #$253B
out (c), a          ; write value
```

Uses the file-scope scratch-byte pattern (`rock_nr_reg`, `rock_nr_val`) to pass arguments through statics rather than relying on SDCC's calling convention inside the naked `__asm` block.

`cpu_speed_set` and `mmu_set` are portable wrappers built on `next_reg_set`/`next_reg_get` — they have identical semantics on both targets because the host shadow file mirrors the same register layout.

### Host path

256-byte `rock_nr_shadow[]` array simulates the register file. `next_reg_set` writes to it; `next_reg_get` reads from it. This makes the component fully round-trip testable on host without any hardware.

### Key register constants

| Rock call | Next Register | Notes |
|-----------|--------------|-------|
| `cpu_speed_set/get` | `$07` (7) | Low 2 bits = turbo mode |
| `mmu_set(slot, page)` | `$50 + slot` (80–87) | One register per MMU slot |

See [[targets/zxn/zxn-memory-paging]] for the full MMU slot and bank model, and [[targets/zxn/zxn-ports-registers]] for the complete Next register index.

## Test Coverage

`test/nextreg_test.rkr` — round-trip read/write for arbitrary registers; `cpu_speed_set/get` with masking; `mmu_set` with valid and out-of-range slots.
