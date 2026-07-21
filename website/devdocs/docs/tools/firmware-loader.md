---
sidebar_position: 5
title: Firmware loader
description: Browser Teensy flasher (WebHID) served at /tools/teensyloader/.
---

# Firmware loader

**Live:** [https://toern.live/tools/teensyloader/](https://toern.live/tools/teensyloader/)  
**Source:** [`tools/teensyloader/`](https://github.com/Soundpauli/toern/tree/main/tools/teensyloader)

Based on the open **Teensy Loader Javascript** stack (WebHID flash + optional serial). Used to put a `.hex` / `.bin` onto a Teensy 4.1 from Chrome/Edge without the desktop Teensy Loader app.

## Typical flow

1. Open the tool in a Chromium desktop browser  
2. Choose the firmware file  
3. Select the Teensy via WebHID  
4. Upload / flash  

## Code / licensing

Upstream-style layout: loader JS, example HTML, AGPL-3.0 (see `LICENSE` in that folder). Treat contributions to this vendored tool accordingly.

## Local run

```bash
cd tools/teensyloader
python3 -m http.server 8000
# open the example / index HTML from http://127.0.0.1:8000/
```

The site copy under `website/tools/teensyloader/` is what Netlify serves at `/tools/teensyloader/`.
