---
sidebar_position: 4
title: SD tool
description: Web Serial and Python CLI for the USB serial file server.
---

# SD tool

**Live:** [https://sdtool.toern.live/](https://sdtool.toern.live/)
**Source:** [`standalone-tools/sd-tool-standalone/`](https://github.com/Soundpauli/toern/tree/main/standalone-tools/sd-tool-standalone)

TŒRN does **not** expose the microSD as USB mass storage. Firmware runs a **USB serial file server** (`toern_sd_serial.ino`) from any screen once a host program has opened the port. **Menu → ETC → SD** shows **SCON**: **WAIT** (yellow) or **OK** (green). A cable with no tool dialog does not enter that poll. The SD tool is the host client.

## Browser (Web Serial)

1. Computer: Chrome or Edge **desktop** → open the site → **Connect**
2. Leave the unit on any screen

Features: browse, upload, download, rename, delete; multi-file / folder drops; ZIP extract in-browser; WAV → **44.1 kHz mono 16-bit** on upload.

**Caveats:** Android Chrome usually cannot see the Teensy port. Opening serial can reboot the Teensy — connect again after it returns.

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

Firmware side: `toern_sd_serial.ino`, polled from `loop()` only while the host serial port is open.

## Local browser and visual theme

`index.html` is the hosted Web Serial client. `explorer.html` is the local Python-server client. Both retain the same controls and use the shared TŒRN theme; the Python server must serve `/tool-theme.css` alongside its existing CSS/favicon assets. Theme source/sync is documented in [Tools overview](./overview).

## Folder uploads and spaced paths

File pickers, folder pickers, ZIP extraction and directory drops normalize to the same upload items. Directory drops support both FileSystemEntry and FileSystemHandle, read every directory batch, and retain empty directories. Ancestor directories are created before files; existing directories are merged. Uploads keep their original SD destination while collection/conversion is in progress and block overlapping uploads.

`PUT <path> <size> <crc32>` keeps its existing wire format. Firmware parses the two numeric suffix fields from the right so paths may contain spaces, and validates the full decimal size and hex CRC before touching the SD card. The greeting advertises `PUT_SPACES`; the browser checks this before uploading any selection containing spaces. Older firmware requires reflashing for those names. Path limits count UTF-8 bytes, and firmware rejects overlong paths instead of silently truncating or splitting a long component.

Run `node tests/test_sd_tool_upload.mjs` and `python3 tests/test_sd_upload_protocol.py` from the repo root for the browser and firmware regressions.

Large ZIP batches use a lazy file-like entry (`name`, `size`, `arrayBuffer`) rather than materializing the entire archive and every inflated file. Only the end record and central directory are indexed up front; payload slices are decompressed and checked against the entry size/CRC when each upload begins. Full-archive Chrome validation of the supplied 175-file archive completed successfully. The traced hardware stall was missing USB receive bytes: the firmware waited for the rest of a block until its 60-second IO timeout. The receiver now buffers a full 8 KB host block before SD writes and consumes the core single-byte read API to avoid receive-ring updates after the bulk reader re-enables interrupts. All 175 converted archive files (63,292,654 bytes) passed upload and byte-for-byte hardware readback using ordinary PUT framing, followed by cleanup (297 seconds). `PUTTRACE` and `SDINFO` remain opt-in diagnostics without altering ordinary PUT framing.
