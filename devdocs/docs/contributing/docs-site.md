---
sidebar_position: 4
title: This docs site
description: Where the Docusaurus app lives, how docs.toern.live is published, and how to edit it.
---

# This docs site

These pages are a **Docusaurus** app in repo-root **`devdocs/`**. Production is **[docs.toern.live](https://docs.toern.live/)** on the tyng.app host (same pattern as handbook / sim / tools). The marketing Netlify site only **links / redirects** to it — it does not embed a docs build.

## Layout

| Path | Role |
|------|------|
| [`devdocs/`](https://github.com/Soundpauli/toern/tree/main/devdocs) | Docusaurus source (Markdown, config, sidebar) |
| Public URL | **https://docs.toern.live/** (`baseUrl` = `/`) |
| Server path | `/var/www/docs.toern.live` (Apache; DNS `docs.toern.live` → `docs.tyng.app`) |

Do **not** copy a build into `website/` or deploy docs as part of the main Netlify site. The operator handbook is separate: repo [`handbook/`](https://github.com/Soundpauli/toern/tree/main/handbook) → [handbook.toern.live](https://handbook.toern.live/).

## Local preview

```bash
cd devdocs
npm install
npm start
```

Open the URL printed in the terminal (routes are at the site root, e.g. `/architecture/overview`).

## Production build / deploy

```bash
cd devdocs
npm install
npm run build
rsync -avz --delete build/ root@tyng.app:/var/www/docs.toern.live/
```

## Editing content

1. Edit Markdown under `devdocs/docs/`
2. Sidebar order: `devdocs/sidebars.js`
3. Site config / `url` / `baseUrl`: `devdocs/docusaurus.config.js`
4. Preview with `npm start`, then commit the **source** under `devdocs/`

## Related

- Operator handbook (device usage): repo `handbook/` / [handbook.toern.live](https://handbook.toern.live/)
- Firmware sketch: repo root `toern.ino` + `toern_*.ino`
