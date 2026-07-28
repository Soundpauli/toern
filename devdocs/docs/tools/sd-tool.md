---
sidebar_position: 4
title: SD tool
description: Web Serial and Python CLI for transferring files while Menu → ETC → SD is open.
---

# SD tool

**Live:** [https://sdtool.tyng.app/](https://sdtool.tyng.app/)  
**Source:** [`standalone-tools/sd-tool-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/sd-tool-standalone)

TŒRN does **not** expose the microSD as USB mass storage. While **Menu → ETC → SD** is open, firmware runs a **USB serial file server** (`toern_sd_serial.ino`). The SD tool is the host client.

## Browser (Web Serial)

1. Device: **Menu → ETC → SD** (WAIT → OK when connected)  
2. Computer: Chrome or Edge **desktop** → open the site → **Connect**  
3. Stay on ETC → SD for the whole session  

Features: browse, upload, download, rename, delete; multi-file / folder drops; ZIP extract in-browser; WAV → **44.1 kHz mono 16-bit** on upload.

**Caveats:** Android Chrome usually cannot see the Teensy port. Opening serial can reboot the Teensy — re-enter ETC → SD if needed.

See also firmware notes: [SD & storage](../subsystems/sd-storage).

## Python CLI / local web

```bash
cd standalone-tools/sd-tool-standalone
pip install pyserial
python3 toern_sd.py ports
python3 toern_sd.py -p /dev/cu.usbmodemXXXX list /
python3 toern_sd.py -p /dev/cu.usbmodemXXXX put ./kick.wav /samples/kick.wav
python3 toern_sd.py web   # optional UI at http://127.0.0.1:8787
```

Requires a USB type that includes **Serial**.

## Code map

| File | Role |
|------|------|
| `index.html` / `styles.css` | Hosted Web Serial UI |
| `explorer.html` | UI used by local `web` mode |
| `toern_sd.py` | CLI + local server; ACK-framed `GET` / `PUT` / list / mkdir / rm |

Firmware side: `toern_sd_serial.ino`, enabled from `loop()` when the ETC → SD menu page is active.
