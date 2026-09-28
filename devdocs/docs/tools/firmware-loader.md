---
sidebar_position: 5
title: Firmware loader
description: Browser Teensy flasher (WebHID) at firmware.tyng.app. GPL-3.0.
---

# Firmware loader

**Live:** [https://firmware.tyng.app/](https://firmware.tyng.app/)  
**Source:** [`standalone-tools/teensyloader-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/teensyloader-standalone)

Based on **Teensy Loader Javascript** (WebHID flash + optional serial). Used to put a `.hex` / `.bin` onto a Teensy 4.1 from Chrome/Edge without the desktop Teensy Loader app. The same upstream tree is also kept at `tools/teensyloader/`.

## Typical flow

1. Open the tool in a Chromium desktop browser  
2. Choose the firmware file  
3. Select the Teensy via WebHID  
4. Upload / flash  

## Code / licensing

`Teensy-Loader.js` is from [coelacant1/Teensy-Loader-Javascript](https://github.com/coelacant1/Teensy-Loader-Javascript) and is **GPL-3.0**. That license covers `standalone-tools/teensyloader-standalone/` and `tools/teensyloader/` only. The rest of this repository stays MIT. See `LICENSE` in those folders.

`public/toern_firmware_3.0.ino.hex` is the frozen 3.0 build. A later firmware build copies the new hex to `public/toern_firmware_beta.ino.hex` only.

## Local run

```bash
cd standalone-tools/teensyloader-standalone
python3 -m http.server 8000
```
