---
sidebar_position: 4
title: This docs site
description: Where the Docusaurus app lives, how /docs is published, and how to edit it.
---

# This docs site

These pages are a **Docusaurus** app. Source lives at the repo root; the built site is copied into `website/docs/` for deploy.

## Layout

| Path | Role |
|------|------|
| [`devdocs/`](https://github.com/Soundpauli/toern/tree/main/devdocs) | Docusaurus source (Markdown, config, sidebar) |
| `website/docs/` | **Built** static output copied in for deploy (not edited by hand) |
| Public URL | **https://toern.live/docs/** (`baseUrl` = `/docs/`) |

`website/` is local/deploy-only (gitignored). Copy the built docs into `website/docs/` when publishing. The operator handbook is **not** copied into the website; it lives in repo [`handbook/`](https://github.com/Soundpauli/toern/tree/main/handbook) and is served at [handbook.toern.live](https://handbook.toern.live/).

## Local preview

```bash
cd devdocs
npm install
npm start
```

Open the URL printed in the terminal (paths are under `/docs/…`).

## Production build

```bash
cd devdocs
npm install
npm run build
```

Then copy `devdocs/build/` → `website/docs/` (manual copy for deploy).

If you use the local website package, `cd website && npm run build:docs` runs the same build/copy via `scripts/sync-docs.js` against `../devdocs`.

## Editing content

1. Edit Markdown under `devdocs/docs/`
2. Sidebar order: `devdocs/sidebars.js`
3. Site config / `baseUrl`: `devdocs/docusaurus.config.js`
4. Preview with `npm start`, then commit the **source** under `devdocs/` (not anything under `website/`)

## Related

- Operator handbook (device usage): repo `handbook/` / [handbook.toern.live](https://handbook.toern.live/)
- Firmware sketch: repo root `toern.ino` + `toern_*.ino`
