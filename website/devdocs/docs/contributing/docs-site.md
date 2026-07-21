---
sidebar_position: 3
title: This docs site
description: Where the Docusaurus app lives, how /docs is published on Netlify, and how to edit it.
---

# This docs site

These pages are a **Docusaurus** app. Source and publish paths matter for Netlify.

## Layout

| Path | Role |
|------|------|
| [`website/devdocs/`](https://github.com/Soundpauli/toern/tree/main/website/devdocs) | Docusaurus source (Markdown, config, sidebar) |
| `website/docs/` | **Built** static output (generated; not edited by hand) |
| `website/scripts/sync-docs.js` | Builds the app and copies `devdocs/build` → `website/docs` |
| Public URL | **https://toern.live/docs/** (`baseUrl` = `/docs/`) |

The app lives **under `website/`** on purpose: Netlify’s build base directory is `website`, so a repo-root sibling like `../devdocs` is outside the tree and fails the cloud build. Keeping sources at `website/devdocs` fixes that.

Most of `website/` stays local/deploy-only (gitignored). Exceptions are tracked so Netlify can build `/docs`:

- `website/devdocs/**`
- `website/package.json` / `package-lock.json`
- `website/scripts/**`
- `website/netlify.toml`

## Local preview

```bash
cd website/devdocs
npm install
npm start
```

Open the URL printed in the terminal (paths are under `/docs/…`).

## Production build (same as Netlify)

From the website package:

```bash
cd website
npm run build:docs
```

That runs `node scripts/sync-docs.js --build`, which:

1. `npm install` + `npm run build` inside `website/devdocs`
2. Copies the Docusaurus `build/` folder to `website/docs/`

A full site build (`npm run build` in `website/`) includes `build:docs` as the last step.

## Editing content

1. Edit Markdown under `website/devdocs/docs/`  
2. Sidebar order: `website/devdocs/sidebars.js`  
3. Site config / `baseUrl`: `website/devdocs/docusaurus.config.js`  
4. Preview with `npm start`, then commit the **source** under `website/devdocs/` (not the generated `website/docs/` output)

## Related

- Operator handbook (device usage): repo `handbook/` / site `/handbook/`  
- Firmware sketch: repo root `toern.ino` + `toern_*.ino`
