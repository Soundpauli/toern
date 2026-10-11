---
sidebar_position: 3
title: Browser simulator
description: Browser UI, approximate audio, panel layouts and pattern transfer.
---

# Browser simulator

**Live:** [sim.tyng.app](https://sim.tyng.app/)
**Source:** [`standalone-tools/web-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/web-standalone)

The simulator provides a browser version of the grid, four encoders, touch gestures and menus. Its audio engine and timing approximate the device; it is not the Teensy firmware running in an emulator. Use the [first-beat handbook](https://toern.live/handbook/first-beat.html) for the physical instrument.

## Controls and labels

- Click the grid to activate browser audio; Space starts/pauses playback.
- Mouse controls operate the encoder UI; the page lists its browser shortcuts.
- Panel layout selects **16×16** or **32×16**; this is not a firmware-version selector.
- Menu labels match the device: **PATT, KIT, SMP, REC, SEL, FOLD, OVWR, TRIG**.
- PATT occupancy uses colour without USED/NEW text.
- The INFO version string in `src/firmware/screens.js` should match `VERSION` in the repository’s `toern.ino`; currently **v3.01f-T2**.

## Transfer boundaries

Pattern transfer covers grid notes and tempo rather than complete sample kits and device sound settings. Check the transfer dialog for whether the selected operation changes device RAM or writes a numbered file. Browser-saved patterns live in local browser storage; they are separate from the unit’s microSD files.

## Local run and build

```bash
cd standalone-tools/web-standalone
npm ci
npm run dev
npm run build
```

Deploy `dist/` after a successful build. `src/firmware/` provides browser state/drawing; `src/audio/engine.js` provides the browser audio implementation. A version-label update does not establish audio/timing parity with hardware.
