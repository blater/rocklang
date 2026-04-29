---
title: RTL Component — cls (clear screen)
category: generator
tags: [rtl, cls, display, screen, zxn, host]
sources: []
updated: 2026-04-25
status: current
---

# RTL Component: cls

`cls()` clears the display. It is a zero-argument void builtin.

## Rock API

```rock
cls();
```

## Implementation

**Source:** `src/lib/cls.{h,c}`

| Target | Mechanism |
|--------|-----------|
| ZXN | Calls ROM entry `$0DAF` (RST/CALL CLS). Clears the upper screen and fills the attribute area with the current `PAPER`/`INK` from the `ATTR_P` system variable. |
| Host (tty) | `tb_clear()` + `tb_present()` — clears the termbox2 back buffer and redraws immediately. Gated on `host_caps.print_at`. |
| Host (piped) | `putchar('\f')` — sends a form-feed to stdout so piped output stays deterministic. |

The host fallback branch is selected by `host_caps.print_at` (set by `rock_rtl_init()` based on `isatty(STDOUT_FILENO)`). See [[rtl-host-caps]] for the initialisation flow.

## Notes

- On ZXN, `cls()` respects the current `ATTR_P` attribute state. Combined with [[rtl-ink-paper]] setters, the cleared screen takes the ink/paper currently set.
- No return value; no parameters.

## Test Coverage

`test/cls_test.rkr` — smoke test only (stdout piped in test harness → fallback branch).

See [[rtl-print-at]] for positioned text and [[rtl-ink-paper]] for attribute state.
