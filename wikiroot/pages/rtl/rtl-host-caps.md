---
title: "RTL Host Capability Layer"
category: concepts
tags: [rtl, runtime, host, capabilities, lifecycle, termbox2]
sources: [src/lib/host_caps.h, src/lib/host_caps.c, src/generator.c]
updated: 2026-04-13
status: current
---

# RTL Host Capability Layer

`host_caps` is the single place where the Rock RTL probes its environment, opens host terminals, and installs `atexit` teardown hooks. Every RTL component that has different fidelity between the real ZXN target and the host (gcc) build exposes a flag here and reads it on each call. Components do **not** call `isatty`, `tb_init`, `atexit`, or any other lifecycle primitive themselves.

## API

```c
typedef struct rock_host_caps {
  byte print_at;   /* positioned text output renders faithfully */
  /* Future: byte keyboard; byte border; byte sound; ... */
} rock_host_caps;

extern rock_host_caps host_caps;

void rock_rtl_init(void);
void rock_rtl_shutdown(void);
```

`rock_rtl_init()` is called exactly once at program startup, and `rock_rtl_shutdown()` exactly once before `return 0`, by the generator-emitted `main()` wrapper. Both calls are inserted in `transpile_fundef` (see `src/generator.c`) immediately around the user's entry-point body, between `fill_cmd_args(argc, argv)` and `kill_compiler_stack()`.

## ZXN implementation

Trivial: every capability is set to 1. We are running on the real machine — every RTL component backed by hardware/ROM is always available at full fidelity, so there is nothing to probe and nothing to tear down.

```c
void rock_rtl_init(void)     { host_caps.print_at = 1; }
void rock_rtl_shutdown(void) { /* nothing */ }
```

## Host (gcc) implementation

Does the real probing. For `print_at` specifically:

1. `isatty(STDOUT_FILENO)` — if stdout is piped (e.g. under `run_tests.sh`, which captures output for assertion comparison), `print_at` is disabled and components fall back to plain-text output.
2. `tb_init()` — if termbox2 cannot open the terminal, `print_at` is disabled.
3. On success, an `atexit` handler is installed to call `tb_present()` + `tb_shutdown()` on program exit, and `host_caps.print_at` is set to 1.

`tb_init()` runs **once per program**, not once per call. The earlier (pre-2026-04-13) `print_at` had a lazy three-state machine inside its host branch which was both slower and harder to reason about; centralising the lifecycle here removes that footgun for every future component.

## Why a single startup hook (and not lazy-init in each component)

- **One probe per resource.** Each underlying handle (tty, audio device, file) is opened exactly once at known time.
- **Predictable teardown.** `atexit` hooks are installed exactly once, in a single place, so component authors cannot forget them or double-register them.
- **No per-call overhead.** Components reduce to a flag read.
- **Testable.** The fallback path is exercised every test run because the harness pipes stdout, so we never ship a regression where the fallback silently broke.

## Build integration

`src/lib/host_caps.c` is part of `RTL_CORE_SRCS` in the `rock` script, so it is always linked on both targets. The host branch `#include`s `termbox2.h`; `src/lib/host/termbox2_impl.c` (in `RTL_HOST_SRCS`) is the single TU that defines `TB_IMPL`.

## Adding a new capability

1. Add a `byte` field to `rock_host_caps` in `src/lib/host_caps.h`.
2. In the host branch of `rock_rtl_init()`, probe the environment and set the flag (and install any `atexit` teardown).
3. In the SDCC branch, set the flag to 1.
4. In your component's host code, gate behaviour on the flag and provide a fallback when it is 0.
5. **Do not** call `atexit`, `isatty`, `tb_init`, or any other lifecycle primitive from the component file.

## See Also

- [[rtl/rtl-overview]] — RTL conventions (capability layer is rule 11)
- [[rtl/rtl-print-at]] — first consumer of `host_caps.print_at`
- [[generator/generator-overview]] — `transpile_fundef` is where the init/shutdown calls are emitted
