# TŒRN code & hardware docs

Developer documentation for how the firmware and rev H hardware fit together.

This is **not** the operator handbook (device usage). That lives in the repo-root [`handbook/`](../handbook/) tree and is published at [handbook.toern.live](https://handbook.toern.live/).

## Layout

| Path | Role |
|------|------|
| `devdocs/` | This Docusaurus app (source of truth in the repo) |
| Public URL | **https://docs.toern.live/** |

Publish by building here and rsyncing `build/` to the tyng.app host. The marketing Netlify site only links / redirects to docs.toern.live — it does not host a nested docs copy.

## Local preview

```bash
cd devdocs
npm install
npm start
```

`url` / `baseUrl` match production: https://docs.toern.live/

## Production build

```bash
cd devdocs
npm install
npm run build
rsync -avz --delete build/ root@tyng.app:/var/www/docs.toern.live/
```

Details: [docs → Contributing → This docs site](./docs/contributing/docs-site.md).
