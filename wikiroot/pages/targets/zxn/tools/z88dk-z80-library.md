---
title: Z88DK z80.h Library
category: targets
tags: [zxn, z80, z88dk, sdcc, z80.h, port-io, interrupts, delay]
sources: [z88dk_z80.md]
updated: 2026-04-11
status: current
---

# Z88DK z80.h Library

Z88DK provides `<z80.h>`, a header of specialized functions for the Z80 microprocessor. Rock programs compiled for the ZXN target can call these functions from `@embed c` blocks or from C helpers.

```c
// In an @embed c block at the top of a Rock file:
#include <z80.h>
```

See [[targets/zxn-z80]] for the overall ZXN compilation setup and [[syntax/embed]] for Rock's `@embed c` syntax.

---

## Timing

### `void z80_delay_ms(uint16_t ms)`

Busy-waits for exactly `ms` milliseconds. Time will be longer if interrupts occur during the wait. Accuracy depends on the `__clock_freq` constant in the target CRT configuration.

### `void z80_delay_tstate(uint16_t tstates)`

Busy-waits for exactly `tstates` Z80 clock cycles. `tstates` must be ≥ 141.

---

## Port I/O

These functions perform Z80 port reads and writes. Prefer `peek`/`poke` (built into Rock) for memory-mapped I/O; use these for port-addressed hardware.

| Function | Description |
|----------|-------------|
| `uint8_t z80_inp(uint16_t port)` | Read one byte from port |
| `void z80_outp(uint16_t port, uint16_t data)` | Write one byte to port |
| `void *z80_inir(void *dst, uint16_t port)` | Block input, increment (`INIR`) |
| `void *z80_indr(void *dst, uint16_t port)` | Block input, decrement (`INDR`) |
| `void *z80_otir(void *src, uint16_t port)` | Block output, increment (`OTIR`) |
| `void *z80_otdr(void *src, uint16_t port)` | Block output, decrement (`OTDR`) |

`z80_inir`/`z80_otir` are efficient for reading/writing a block of bytes to a single port (e.g. streaming sprite data to the ZXN sprite upload port `$5B`).

---

## Memory Access Macros

These expand to direct pointer-cast reads/writes — equivalent to Rock's `peek`/`poke` but with explicit width.

| Macro | Width | Direction |
|-------|-------|-----------|
| `z80_bpoke(addr, byte)` | 8-bit | write |
| `z80_wpoke(addr, word)` | 16-bit | write |
| `z80_lpoke(addr, dword)` | 32-bit | write |
| `z80_bpeek(addr)` | 8-bit | read |
| `z80_wpeek(addr)` | 16-bit | read |
| `z80_lpeek(addr)` | 32-bit | read |

---

## Interrupt State

### `uint16_t z80_get_int_state(void)`
### `void z80_set_int_state(uint16_t state)`

Save and restore the interrupt enable state (I flag). Always disable interrupts before modifying interrupt mode. These do not themselves enable or disable interrupts.

---

## IM2 Interrupt Management

These functions help set up Z80 IM2 (interrupt mode 2) with a managed vector table. See [[targets/zxn/zxn-interrupts]] for ZXN's hardware IM2 variant.

### `void im2_init(void *im2_table_address)`

Sets the IM2 vector table address and switches the CPU to IM2 mode. The least-significant byte of `im2_table_address` is discarded — the table must be on an exact 256-byte page boundary.

### `void *im2_install_isr(uint16_t vector, void (*isr)(void))`

Install an ISR at the given IM2 vector. Returns the previously registered ISR. By convention, use even vectors only (Zilog peripherals produce even vectors).

### `void *im2_create_generic_isr(uint16_t num_callbacks, void *address)`

Create a generic ISR in RAM at `address` that can call up to `num_callbacks` (< 128) registered subroutines in sequence. Memory required: `18 + num_callbacks * 2` bytes. The generic ISR saves all registers before calling callbacks and restores them after.

Returns the next free address after the installed ISR.

### `void *im2_create_generic_isr_8080(uint16_t num_callbacks, void *address)`

Same as `im2_create_generic_isr`, but saves only the main register set (AF, BC, DE, HL), not the alternate set or index registers. Faster but requires callbacks to preserve IX, IY, AF', BC', DE', HL' themselves.

### `void im2_append_generic_callback(uint16_t vector, void (*callback)(void))`
### `void im2_prepend_generic_callback(uint16_t vector, void (*callback)(void))`
### `int im2_remove_generic_callback(uint16_t vector, void (*callback)(void))`

Add or remove a callback subroutine from a generic ISR's dispatch list.

---

## Usage from Rock

Access these functions via `@embed c`:

```rock
@embed c
#include <z80.h>

void wait_10ms(void) {
    z80_delay_ms(10);
}

uint8_t read_port(uint16_t port) {
    return z80_inp(port);
}
@end c

sub main(): void {
  wait_10ms();
}
```

On host builds the `@embed c` block is compiled by gcc; the `z80.h` header is Z88DK-specific and will not be available. Wrap host-incompatible code in `#ifdef __SDCC` if the file must compile on both targets.

## See Also

- [[syntax/embed]] — `@embed c` and `@embed asm` Rock syntax
- [[targets/zxn-z80]] — ZXN target overview
- [[targets/zxn/tools/z88dk-inline-asm]] — Inline assembly and calling conventions
- [[targets/zxn/zxn-interrupts]] — ZXN hardware interrupt system
