---
sidebar_position: 5
title: Firmware loader
description: Browser Teensy flasher (WebHID) at firmware.tyng.app. GPL-3.0.
---

# Firmware loader

**Live:** [https://firmware.tyng.app/](https://firmware.tyng.app/)
**Source:** [`tools/teensyloader/`](https://github.com/Soundpauli/toern/tree/main/tools/teensyloader)

Based on **Teensy Loader Javascript** (WebHID flash + optional serial). The current UI flashes its bundled stable **v3.0** or **Beta** HEX preset onto a Teensy 4.1 from Chrome/Edge desktop. It does not expose a custom HEX/BIN file picker. The library supports firmware-file parsing, but the UI deliberately uses those two presets.

## Typical flow

1. Open the tool in a Chromium desktop browser
2. Choose stable v3.0 or Beta
3. Upload. A running sketch is rebooted into the bootloader by opening its serial port at 134 baud. The red LED blinks, then WebHID flashes it. The physical program button is the fallback when no sketch is running.

## Code / licensing

`Teensy-Loader.js` is from [coelacant1/Teensy-Loader-Javascript](https://github.com/coelacant1/Teensy-Loader-Javascript) and is **GPL-3.0**. That license covers `tools/teensyloader/`. Other software in this repository is MIT; the hardware design has the separate license described in [Hardware overview](../hardware/overview). See `tools/teensyloader/LICENSE`.

`public/toern_firmware_3.0.ino.hex` is the frozen 3.0 build. The Beta selector loads `public/toern_firmware_beta.ino.hex`. Merely building the device sketch does not update this asset; copy a tested build explicitly. Do not relabel the frozen stable image as the latest source version.

## Local run

```bash
cd tools/teensyloader
python3 -m http.server 8000
```

## Production build

```bash
cd tools/teensyloader
npm ci
npm run build
```

Deploy the generated `dist/` directory. Vite bundles the selected firmware assets, programming-button illustration, scripts and theme with relative paths so the app can be hosted independently. The plain source also works with the HTTP server shown above. The shared UI theme is copied locally into each standalone tool; see [Tools overview](./overview).
