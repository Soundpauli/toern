import fs from "node:fs";
import path from "node:path";
import { createReadStream } from "node:fs";
import { defineConfig } from "vite";
import { audioImportServer } from "./audioImportServer.js";

function walk(dir, base = "") {
  const out = [];
  for (const name of fs.readdirSync(dir)) {
    if (name.startsWith(".")) continue;
    const rel = base ? `${base}/${name}` : name;
    const full = path.join(dir, name);
    if (fs.statSync(full).isDirectory()) out.push(...walk(full, rel));
    else if (/\.(wav|txt)$/i.test(name)) out.push({ rel, size: fs.statSync(full).size });
  }
  return out;
}

function copyDir(src, dest) {
  fs.mkdirSync(dest, { recursive: true });
  for (const name of fs.readdirSync(src)) {
    if (name.startsWith(".")) continue;
    const from = path.join(src, name);
    const to = path.join(dest, name);
    if (fs.statSync(from).isDirectory()) copyDir(from, to);
    else fs.copyFileSync(from, to);
  }
}

function sdFiles() {
  const root = path.resolve("SD-CARD-CONTENT");
  function serve(req, res, next) {
    let rel = decodeURIComponent((req.url || "").split("?")[0]);
    rel = rel.replace(/^\/sd\/?/, "").replace(/^\/+/, "");
    const file = path.resolve(root, rel);
    if (!file.startsWith(root) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) return next();
    res.setHeader("Content-Type", file.endsWith(".wav") ? "audio/wav" : "application/octet-stream");
    createReadStream(file).pipe(res);
  }
  return {
    name: "sd-files",
    resolveId(id) {
      if (id === "virtual:sd") return "\0virtual:sd";
    },
    load(id) {
      if (id !== "\0virtual:sd") return;
      return `export default ${JSON.stringify(walk(root))}`;
    },
    configureServer(server) {
      server.middlewares.use("/sd", serve);
    },
    configurePreviewServer(server) {
      server.middlewares.use("/sd", serve);
    },
    closeBundle() {
      const out = path.resolve("dist/sd");
      if (!fs.existsSync(root)) return;
      fs.rmSync(out, { recursive: true, force: true });
      copyDir(root, out);
    },
  };
}

export default defineConfig({
  root: ".",
  base: "/",
  server: { port: 5173, strictPort: true },
  plugins: [sdFiles(), audioImportServer()],
});
