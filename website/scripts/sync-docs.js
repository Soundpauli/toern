#!/usr/bin/env node
/**
 * Build Docusaurus and copy into website/docs for Netlify (/docs/).
 *
 * Docs live at website/devdocs (same Netlify base directory as this site),
 * so the build does not depend on a sibling outside the website tree.
 */
const { spawnSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const websiteDir = path.join(__dirname, "..");
const shouldBuild = process.argv.includes("--build");

function resolveDevdocs() {
  const candidates = [
    path.join(websiteDir, "devdocs"), // website/devdocs — Netlify base = website
    path.join(websiteDir, "..", "devdocs"), // legacy monorepo sibling
    path.join(process.cwd(), "devdocs"),
  ];
  for (const dir of candidates) {
    if (fs.existsSync(path.join(dir, "package.json"))) {
      return path.resolve(dir);
    }
  }
  return null;
}

function run(cmd, args, cwd) {
  const result = spawnSync(cmd, args, {
    cwd,
    stdio: "inherit",
    shell: process.platform === "win32",
  });
  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

const srcRoot = resolveDevdocs();
if (!srcRoot) {
  console.error("[sync-docs] Could not find devdocs/package.json.");
  console.error(`[sync-docs] Expected at: ${path.join(websiteDir, "devdocs")}`);
  console.error("[sync-docs] websiteDir=", websiteDir, "cwd=", process.cwd());
  try {
    console.error("[sync-docs] websiteDir listing:", fs.readdirSync(websiteDir).join(", "));
  } catch (e) {
    console.error("[sync-docs] cannot list websiteDir:", e.message);
  }
  process.exit(1);
}

console.log(`[sync-docs] Using ${srcRoot}`);

if (shouldBuild) {
  run("npm", ["install"], srcRoot);
  run("npm", ["run", "build"], srcRoot);
}

const src = path.join(srcRoot, "build");
const dest = path.join(websiteDir, "docs");

if (!fs.existsSync(src)) {
  console.error(`[sync-docs] Missing ${src}. Run with --build or build devdocs first.`);
  process.exit(1);
}

fs.rmSync(dest, { recursive: true, force: true });
fs.cpSync(src, dest, { recursive: true });
console.log(`[sync-docs] Copied ${src} → ${dest}`);
