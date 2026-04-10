---
title: ZXN Memory Map and Paging
category: targets
tags: [zxn, memory, paging, mmu, banks, slots]
sources: [zxnext_guide.md]
updated: 2026-04-10
status: current
---

# ZXN Memory Map and Paging

The Z80 CPU has a 16-bit address bus and can only access 64KB at once. The ZX Spectrum Next extends this through a **banking** system: physical memory is divided into fixed-size **banks** (also called pages) that are swapped into **slots** in the CPU address space. A Rock implementer needs this to work with Layer 2, sprite patterns, or any data larger than 64KB.

## Banks and Slots

Two bank sizes are available:
- **16K banks** — legacy scheme inherited from 128K Spectrum. Numbered from 0.
- **8K banks** — Next MMU mode. More flexible; supports full 2048K on expanded hardware. 16K bank N maps to 8K banks N×2 and N×2+1.

The first 256KB is reserved for ROMs and firmware. **Bank 0 starts at absolute address `$40000`**.

Address formula:
- 16K bank N at absolute: `$40000 + N × 16384`
- 8K bank N at absolute: `$40000 + N × 8192`

![Banks and slots diagram](../../../raw/images/zxnext_guide_p046_f1.png)
![Default bank traits](../../../raw/images/zxnext_guide_p047_f1.png)

## Legacy Paging Modes (16K)

Inherited from 128K/+2/+3. Three variants:

### 128K Mode

- **Slot 0** (`$0000–$3FFF`): ROM selected by bit 4 of `$7FFD` (2 options)
- **Slot 3** (`$C000–$FFFF`): RAM selected by bits 2–0 of `$7FFD` (8 banks)
- **Extended** (Next): bits 3–0 of `$DFFD` add upper bits → 128 banks for slot 3
- Always store the written value at system variable `$5B5C` when using ROM paging

### +3 Normal Mode

- **Slot 0**: 4 ROMs — bit 2 of `$1FFD` (LSB) + bit 4 of `$7FFD` (MSB)
- **Slot 3**: 8 RAM banks — bits 2–0 of `$7FFD` + bits 3–0 of `$DFFD`
- Store `$1FFD` writes to `$5B67`; `$7FFD` writes to `$5B5C`

### +3 All-RAM (Special/CP/M) Mode

- Bit 0 of `$1FFD` = 1 enables All-RAM; bits 2–1 select configuration
- All 4 slots selectable independently
- Store `$1FFD` writes to `$5B67`

![128K paging](../../../raw/images/zxnext_guide_p048_f1.png)
![+3 paging](../../../raw/images/zxnext_guide_p048_f2.png)

## Next MMU Paging Mode (8K) ← Preferred

The most flexible mode. Maps 8K banks into any of 8 CPU slots. The **only** mode supporting the full 2MB on expanded hardware.

| Slot | CPU Range | MMU Register |
|------|-----------|--------------|
| MMU0 | `$0000–$1FFF` | `$50` |
| MMU1 | `$2000–$3FFF` | `$51` |
| MMU2 | `$4000–$5FFF` | `$52` |
| MMU3 | `$6000–$7FFF` | `$53` |
| MMU4 | `$8000–$9FFF` | `$54` |
| MMU5 | `$A000–$BFFF` | `$55` |
| MMU6 | `$C000–$DFFF` | `$56` |
| MMU7 | `$E000–$FFFF` | `$57` |

**Special case for `$50`/`$51`**: Writing `$FF` to either auto-pages in the ROM. The low/high 8K of ROM is selected based on the slot number.

**Example — write 10 bytes to 8K bank 30, mapped into slot 5 (`$A000`):**
```asm
NEXTREG $55, 30   ; map 8K bank 30 into slot 5
LD DE, $A000      ; slot 5 base address
LD A, 0
LD B, 10
loop:
  LD (DE), A
  INC A
  INC DE
  DJNZ loop
```

## Interaction Between Paging Modes

Legacy and MMU modes are interchangeable; the **most recent write wins**. A legacy 16K change affects 2 consecutive 8K banks.

**Key fixed mappings:**
- **ROM**: always `$0000–$3FFF`; can only be remapped using +3 All-RAM or MMU mode. The IM1 interrupt handler is at `$0038` — remapping ROM requires redirecting interrupts.
- **ULA**: always reads from 16K bank 5 (slots 2–3, `$4000–$7FFF`). Bit 3 of `$7FFD` redirects to bank 7 (shadow screen).
- **Layer 2**: has its own access port `$123B`; also supports double-buffering via `$13`.

![Paging interaction](../../../raw/images/zxnext_guide_p052_f1.png)

## Registers

**Memory Paging Control `$7FFD`**

| Bit | Description |
|-----|-------------|
| 7–6 | Extra bank bits (Pentagon 512K/1024K) |
| 5 | Lock — 1 locks paging (irreversible on 128K until reset) |
| 4 | ROM select (128K: 0=128K editor, 1=48K BASIC; +3: LSB of ROM select) |
| 3 | ULA shadow screen: 0=bank 5, 1=bank 7 |
| 2–0 | RAM bank for slot 3 (`$C000`) — 16K bank number |

**+3 Memory Paging Control `$1FFD`**

| Bit | Description |
|-----|-------------|
| 4 | Normal mode: MSB of ROM select |
| 3 | Special mode: high bit of memory config |
| 2 | Special mode: low bit of memory config; or MSB of ROM select |
| 0 | Paging mode: 0=normal, 1=special (All-RAM) |

**Next Memory Bank Select `$DFFD`**

| Bit | Description |
|-----|-------------|
| 4 | Pentagon 512K/1024K enable |
| 3–0 | MSBs of 16K RAM bank selected in `$7FFD` |

**Memory Management Slot Registers `$50`–`$57`**
- Each register: bits 7–0 = 8K bank number mapped to corresponding slot

**Memory Mapping `$8E`** — shortcut combining `$1FFD`/`$7FFD`/`$DFFD`

| Bit | Description |
|-----|-------------|
| 7 | Access to bit 0 of `$DFFD` |
| 6–4 | Access to bits 2–0 of `$7FFD` |
| 3 | 1=change RAM bank in MMU6/7, `$7FFD`, `$DFFD`; 0=no change |
| 2 | 0=normal paging, 1=+3 special All-RAM |
| 1 | Access to bit 2 of `$1FFD` |
| 0 | Normal: bit 4 of `$7FFD`; Special: bit 1 of `$1FFD` |

![Paging registers](../../../raw/images/zxnext_guide_p053_f1.png)

## See Also

- [[targets/zxn-hardware]] — layer stack and bank overview
- [[targets/zxn/zxn-layer2]] — Layer 2 uses its own bank paging
- [[targets/zxn/zxn-ports-registers]] — full register index
