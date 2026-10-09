/** Dev/preview-server bridge to audio_import/toern_analyze.py.
 *
 *  POST /api/audio/analyze?name=song.mp3   body = file bytes → { hash, state }
 *  GET  /api/audio/status/<hash>                              → { state, pct, msg, error }
 *  GET  /api/audio/file/<hash>/<path>      analysis.json, samples/N.wav, source.*
 *
 *  Results are cached per file hash in <audio_import>/cache/<hash>/.
 */
import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { spawn } from "node:child_process";

const AUDIO_DIR = path.resolve(process.env.TOERN_AUDIO_IMPORT_DIR || "../../audio_import");
const CACHE = path.join(AUDIO_DIR, "cache");
const jobs = new Map();
// Must match ANALYSIS_VERSION in toern_analyze.py; older caches are re-analyzed.
const ANALYSIS_VERSION = 4;

function python() {
  const venv = path.join(AUDIO_DIR, ".venv/bin/python");
  return fs.existsSync(venv) ? venv : "python3";
}

function json(res, code, body) {
  res.statusCode = code;
  res.setHeader("Content-Type", "application/json");
  res.end(JSON.stringify(body));
}

function readBody(req) {
  return new Promise((resolve, reject) => {
    const chunks = [];
    req.on("data", (c) => chunks.push(c));
    req.on("end", () => resolve(Buffer.concat(chunks)));
    req.on("error", reject);
  });
}

function status(hash) {
  const dir = path.join(CACHE, hash);
  const job = jobs.get(hash);
  const file = path.join(dir, "analysis.json");
  if (!job && fs.existsSync(file)) {
    try {
      if (JSON.parse(fs.readFileSync(file, "utf8")).version >= ANALYSIS_VERSION) return { state: "done", pct: 100, msg: "Done" };
    } catch { /* re-analyze */ }
  }
  if (job) return { state: job.error ? "error" : "running", pct: job.pct, msg: job.msg, error: job.error };
  return { state: "missing" };
}

function startJob(hash, source) {
  const dir = path.join(CACHE, hash);
  const job = { pct: 0, msg: "Starting analysis", error: null, log: "" };
  jobs.set(hash, job);
  const child = spawn(python(), ["toern_analyze.py", source, "--out", dir], {
    cwd: AUDIO_DIR,
    env: { ...process.env, PYTHONUNBUFFERED: "1", PATH: `${path.join(AUDIO_DIR, ".venv/bin")}:${process.env.PATH}` },
  });
  const onData = (buf) => {
    const text = buf.toString();
    job.log = (job.log + text).slice(-4000);
    for (const line of text.split(/[\r\n]+/)) {
      const m = /^@@PROGRESS (\d+) (.*)$/.exec(line.trim());
      if (m) { job.pct = Number(m[1]); job.msg = m[2]; continue; }
      const d = /(\d+)%\|/.exec(line);
      if (d && job.msg.startsWith("Separating")) job.pct = 5 + Math.round(Number(d[1]) * 0.4);
    }
  };
  child.stdout.on("data", onData);
  child.stderr.on("data", onData);
  child.on("close", (code) => {
    if (code === 0 && fs.existsSync(path.join(dir, "analysis.json"))) { jobs.delete(hash); return; }
    const err = /ERROR: (.*)/.exec(job.log);
    job.error = err ? err[1] : `Analyzer exited with ${code}`;
  });
  child.on("error", (e) => { job.error = `Cannot start Python: ${e.message}`; });
}

async function handle(req, res, next) {
  const url = new URL(req.url, "http://x");
  // Mounted at /api/audio, so connect has already stripped that prefix.
  const parts = url.pathname.split("/").filter(Boolean);
  try {
    if (parts[0] === "analyze" && req.method === "POST") {
      const body = await readBody(req);
      if (!body.length) return json(res, 400, { error: "empty upload" });
      const hash = crypto.createHash("sha1").update(body).digest("hex").slice(0, 16);
      const ext = (path.extname(url.searchParams.get("name") || "") || ".mp3").toLowerCase().replace(/[^.a-z0-9]/g, "");
      const dir = path.join(CACHE, hash);
      fs.mkdirSync(dir, { recursive: true });
      const source = path.join(dir, `source${ext}`);
      if (!fs.existsSync(source)) fs.writeFileSync(source, body);
      fs.writeFileSync(path.join(dir, "name.txt"), url.searchParams.get("name") || `source${ext}`);
      let st = status(hash);
      if (st.state === "missing" || st.state === "error") {
        startJob(hash, source);
        st = status(hash);
      }
      return json(res, 200, { hash, ...st });
    }
    if (parts[0] === "status" && parts[1]) return json(res, 200, status(parts[1].replace(/[^0-9a-f]/g, "")));
    if (parts[0] === "file" && parts[1]) {
      const dir = path.join(CACHE, parts[1].replace(/[^0-9a-f]/g, ""));
      const file = path.resolve(dir, parts.slice(2).join("/"));
      if (!file.startsWith(dir + path.sep) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) return next();
      const type = file.endsWith(".json") ? "application/json" : file.endsWith(".wav") ? "audio/wav" : "application/octet-stream";
      res.setHeader("Content-Type", type);
      res.setHeader("Cache-Control", "no-store");
      fs.createReadStream(file).pipe(res);
      return;
    }
    if (parts[0] === "ping") return json(res, 200, { ok: true, dir: AUDIO_DIR });
    next();
  } catch (err) {
    json(res, 500, { error: String(err?.message || err) });
  }
}

export function audioImportServer() {
  return {
    name: "toern-audio-import",
    configureServer(server) { server.middlewares.use("/api/audio", handle); },
    configurePreviewServer(server) { server.middlewares.use("/api/audio", handle); },
  };
}
