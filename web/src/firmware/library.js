import listing from "virtual:sd";

function urlFor(rel) {
  return `/sd/${rel.split("/").map(encodeURIComponent).join("/")}`;
}

const files = listing.map((item) => {
  const rel = typeof item === "string" ? item : item.rel;
  return { rel, url: urlFor(rel), name: rel.split("/").pop(), size: item.size || 0 };
});

export function listDir(dir, { parent = true } = {}) {
  const prefix = dir ? `${dir}/` : "";
  const dirs = new Set();
  const found = [];
  for (const file of files) {
    if (prefix && !file.rel.startsWith(prefix)) continue;
    if (!prefix && file.rel.includes("/")) {
      dirs.add(file.rel.split("/")[0]);
      continue;
    }
    const rest = prefix ? file.rel.slice(prefix.length) : file.rel;
    if (!rest) continue;
    if (rest.includes("/")) dirs.add(rest.split("/")[0]);
    else found.push(file);
  }
  const items = [];
  if (dir && parent) items.push({ name: "[../]", type: "up" });
  for (const name of [...dirs].sort((a, b) => a.localeCompare(b))) items.push({ name, type: "dir" });
  found.sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true }));
  for (const file of found) items.push({ name: file.name, type: "file", url: file.url, rel: file.rel, size: file.size || 0 });
  return items;
}

export function patternUrl(slot) {
  const name = slot === 0 ? "autosaved.txt" : `${slot}.txt`;
  return files.find((file) => file.rel === name)?.url || null;
}

export function packWavs(slot) {
  const urls = [];
  for (let voice = 1; voice <= 8; voice++) {
    urls.push(files.find((file) => file.rel === `${slot}/${voice}.wav`)?.url || null);
  }
  return urls;
}

export async function parsePattern(url) {
  const buf = new Uint8Array(await (await fetch(url)).arrayBuffer());
  const rows = 16;
  const steps = 256;
  if (buf.length < steps * rows * 4) return null;
  const data = [];
  for (let i = 0; i < steps * rows; i++) {
    const o = i * 4;
    const channel = buf[o];
    data.push(channel ? [channel, buf[o + 1], buf[o + 2], buf[o + 3]] : 0);
  }
  return data;
}
