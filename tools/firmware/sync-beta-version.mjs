#!/usr/bin/env node
/**
 * Keep Beta / BETA-MODULR labels aligned with #define VERSION in toern.ino.
 * Writes public/beta-version.json for the loader UI.
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const sketch = join(here, '..', '..', 'platformio', 'toern.ino');
const out = join(here, 'public', 'beta-version.json');

const src = readFileSync(sketch, 'utf8');
const match = src.match(/#define\s+VERSION\s+"([^"]+)"/);
if (!match) {
  console.error(`sync-beta-version: VERSION not found in ${sketch}`);
  process.exit(1);
}

const version = match[1];
writeFileSync(out, `${JSON.stringify({ version }, null, 2)}\n`);
console.log(`sync-beta-version: ${version} → ${out}`);
