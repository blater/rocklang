---
title: ZXN Sound Sample Summary
category: targets
tags: [zxn, samples, sound, ay-3-8912, turbo-sound]
sources: [samples/sound/main.asm]
updated: 2026-04-11
status: current
---

# ZXN Sound Sample Summary

Source: [samples/sound/main.asm](../../../../raw/samples/sound/main.asm)

This sample demonstrates **Turbo Sound Next** setup and direct **AY Register** programming. It configures the active AY chip, enables Turbo Sound-related output registers, then repeatedly walks a compact tune table and writes mixer, tone, noise, channel, and volume values.

## Execution Sequence

1. Start at `$8000`.
2. Select the active AY chip through port `$FFFD` with `%11111101`.
3. Configure stereo/internal-speaker/Turbo Sound behaviour with `NEXTREG $08, %00010010`.
4. Configure mono output for AY0-2 with `NEXTREG $09, %11100000`.
5. Loop over `tune`, calling `playTune`.
6. Stop each pass when the tune reader sees `$FF`.

## Tune Row Format

Each row in `tune` contains six bytes:

| Field | Meaning | AY destination |
|-------|---------|----------------|
| Mixer byte | Enables/disables tone and noise by channel | Register 7 |
| Tone low/high | 12-bit tone period | Registers 0 and 1 |
| Noise period | 5-bit noise period | Register 6 |
| Channel | 0=A, 1=B, 2=C | Selects volume register 8-10 |
| Volume | 0-15 fixed volume | Selected volume register |

The player only writes channel A tone-period registers (`0` and `1`) directly, then chooses the volume register by adding the row's channel field to 8. It is a compact demonstration format, not a general music engine.

## Register Write Helpers

`writeDToAYReg` selects an AY register by writing the register number to `$FFFD`, then writes the value through `$BFFD`. `writeDEToAYReg` writes a 16-bit value across two consecutive AY registers by calling `writeDToAYReg` twice.

The code notes that register numbers must keep bit 7 clear; otherwise `$FFFD` is interpreted as Turbo Sound control rather than AY register select.

## Environment Note

The source warns that CSpect requires OpenAL for this program. Without OpenAL, the emulator can freeze on launch rather than simply running silently.

## Rock Runtime Implications

Rock can expose useful sound support without a full tracker engine:

- a chip-select helper for `$FFFD` control writes;
- a safe AY register writer that rejects register numbers above 13;
- small routines for mixer, tone/noise period, and volume writes;
- optional data-driven playback similar to this tune table.

## See Also

- [[targets/zxn/zxn-sound]] — AY and Turbo Sound reference
- [[targets/zxn/zxn-sample-programs]] — sample-program hub
