---
title: "RTL Host Capability Layer"
category: concepts
tags: [rtl, runtime, host, capabilities, lifecycle, termbox2]
sources: [src/lib/host_caps.h, src/lib/host_caps.c, src/generator.c]
updated: 2026-04-28
status: current
---

# RTL Host Capability Layer

`host_caps` is the single place where the Rock RTL owns host terminal graphics mode and installs `atexit` teardown hooks. Every RTL component that has different fidelity between the real ZXN target and the host (gcc) build exposes a flag here and reads it on each call. Components do **not** call `isatty`, `tb_init`, `atexit`, or any other lifecycle primitive themselves.

## API

```c
typedef struct rock_host_caps {
  byte print_at;   /* positioned text output renders faithfully */
  byte ink;        /* character-cell colour attributes available */
  byte plot;       /* raster pixel plot renders via termbox2 quadrants */
  /* Future: byte keyboard; byte border; byte sound; ... */
} rock_host_caps;

extern rock_host_caps host_caps;

void rock_rtl_init(void);
void rock_rtl_shutdown(void);
void graphics_on(void);
void graphics_off(void);
```

`rock_rtl_init()` is called exactly once at program startup, and `rock_rtl_shutdown()` exactly once before `return 0`, by the generator-emitted `main()` wrapper. Host startup now leaves graphics mode **off** so ordinary command-line output is not captured by termbox2.

Rock source explicitly opts in:

```rock
graphics on;
print(to_byte(10), to_byte(5), "hello");
graphics off;
```

## ZXN implementation

Trivial: every capability is set to 1. We are running on the real machine — every RTL component backed by hardware/ROM is always available at full fidelity, so there is nothing to probe and nothing to tear down. `graphics on;` and `graphics off;` compile to no-op runtime calls on ZXN.

```c
void rock_rtl_init(void)     { host_caps.print_at = 1; }
void rock_rtl_shutdown(void) { /* nothing */ }
void graphics_on(void)       { /* no-op */ }
void graphics_off(void)      { /* no-op */ }
```

## Host (gcc) implementation

Host startup disables all terminal graphics capabilities. The first `graphics on;` statement does the real terminal setup:

1. `isatty(STDOUT_FILENO)` — if stdout is piped, graphics stays disabled and components use their plain-text fallbacks.
2. `tb_init()` — if termbox2 cannot open the terminal, graphics stays disabled.
3. On success, an `atexit` handler is installed once, and `host_caps.print_at`, `host_caps.ink`, and `host_caps.plot` are set to 1.

`graphics off;` calls the same teardown path as program shutdown and resets the capability flags to 0.

## Why a single startup hook (and not lazy-init in each component)

- **Explicit terminal ownership.** Ordinary command-line programs keep normal stdout unless they request graphics mode.
- **One owner per resource.** The host capability layer is still the only place that opens or closes termbox2.
- **Predictable teardown.** `atexit` hooks are installed exactly once, in a single place, so component authors cannot forget them or double-register them.
- **No per-call lifecycle code.** Components reduce to a flag read.
- **Testable.** The fallback path is exercised every test run because the harness pipes stdout, so we never ship a regression where the fallback silently broke.

## Build integration

`src/lib/host_caps.c` is part of `RTL_CORE_SRCS` in the `rock` script, so it is always linked on both targets. The host branch `#include`s `termbox2.h`; `src/lib/host/termbox2_impl.c` (in `RTL_HOST_SRCS`) is the single TU that defines `TB_IMPL`.

## Adding a new capability

1. Add a `byte` field to `rock_host_caps` in `src/lib/host_caps.h`.
2. In `graphics_on()`, probe or initialise whatever host resource backs that flag.
3. In the SDCC branch, set the flag to 1.
4. In your component's host code, gate behaviour on the flag and provide a fallback when it is 0.
5. **Do not** call `atexit`, `isatty`, `tb_init`, or any other lifecycle primitive from the component file.

## See Also

- [[rtl-overview]] — RTL conventions (capability layer is rule 11)
- [[rtl-print-at]] — first consumer of `host_caps.print_at`
- [[generator-overview]] — `transpile_fundef` is where the init/shutdown calls are emitted
