---
sidebar_position: 7
title: Annotate
description: Local tool for handbook-style callout annotations on photos.
---

# Annotate

**Source:** [`tools/annotate/`](https://github.com/Soundpauli/toern/tree/main/tools/annotate)  
Not a public product URL — contributor helper for handbook / docs imagery.

## What it does

- Open or drop a photo  
- Crop / rotate / background fill  
- Place **callout** labels (leader lines + text)  
- Select, align, style (fonts, colors, halos)  
- Export PNG, image-only, self-contained HTML, or JSON to reopen later  

Used for assets like the annotated device overview in the operator handbook.

## Run

```bash
cd tools/annotate
python3 -m http.server 8765
# http://127.0.0.1:8765/
```

Needs `../tool-shell.css` (served from `tools/`).

Details and shortcuts: [`tools/annotate/README.md`](https://github.com/Soundpauli/toern/blob/main/tools/annotate/README.md).
