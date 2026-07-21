---
sidebar_position: 6
title: Color scheme editor
description: Edit LED / UI palette colors and sync live to a connected device.
---

# Color scheme editor

**Live:** [https://toern.live/tools/colorsheme/](https://toern.live/tools/colorsheme/)  
**Source:** [`tools/colorsheme/`](https://github.com/Soundpauli/toern/tree/main/tools/colorsheme) (`index.html` SPA)

## What it does

- Visual editor for TŒRN **palette / channel / UI colors** (previews + pickers)
- Export of scheme data for use with the device
- Optional **Live Sync** over serial to push colors while editing (needs firmware support — UI notes “Requires updated firmware”)

Typo in the folder name (`colorsheme`) is historical; keep paths as-is for existing URLs.

## Local run

Needs the parent tool shell CSS (`tools/tool-shell.css`) and site styles when opened from the website tree. Easiest:

```bash
# from a static server that can resolve ../../styles.css as on the live site
# or open via the website root:
cd website && npx serve -l 3080
# then http://127.0.0.1:3080/tools/colorsheme/
```

## Related firmware

Colors used at runtime are defined largely in [`colors.h`](https://github.com/Soundpauli/toern/blob/main/colors.h) and applied through LED/UI code (`toern_ui.ino`, `toern_leds.ino`). The editor is the host-side way to design palettes without recompiling for every tweak.
