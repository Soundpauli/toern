import assert from "node:assert/strict";
import { File } from "node:buffer";
import { searchBitMidi, sortBitMidi, downloadBitMidi } from "../../tools/sim/src/firmware/bitMidi.js";

globalThis.File ??= File;
const originalFetch = globalThis.fetch;
try {
  let requested;
  globalThis.fetch = async (url, options) => {
    requested = { url: new URL(url), options };
    return Response.json({ result: { total: 2, pageTotal: 3, results: [
      { id: 2, name: "Z song.mid", plays: 20, downloadUrl: "https://untrusted.invalid/file" },
      { id: 1, name: "A song.mid", plays: 5 },
      { id: "bad", name: "Invalid.mid" },
    ] } });
  };
  const result = await searchBitMidi(" Radiohead & alt-J ", 1);
  assert.equal(requested.url.origin, "https://bitmidi.com");
  assert.equal(requested.url.searchParams.get("q"), "Radiohead & alt-J");
  assert.equal(requested.url.searchParams.get("page"), "1");
  assert.equal(requested.options.credentials, "omit");
  assert.equal(result.items.length, 2);
  assert.equal(result.items[0].sourceUrl, "https://bitmidi.com/uploads/2.mid");
  assert.deepEqual(sortBitMidi(result.items, "name").map(x => x.id), [1, 2]);
  assert.deepEqual(sortBitMidi(result.items, "plays").map(x => x.id), [2, 1]);
  assert.deepEqual(sortBitMidi(result.items, "relevance").map(x => x.id), [2, 1]);
  assert.deepEqual(result.items.map(x => x.id), [2, 1], "sorting must preserve search relevance");

  const midi = Uint8Array.from([77, 84, 104, 100, 0, 0, 0, 6, 0, 1, 0, 0, 0, 96]);
  globalThis.fetch = async url => { requested = String(url); return new Response(midi); };
  const file = await downloadBitMidi(result.items[0]);
  assert.equal(requested, "https://bitmidi.com/uploads/2.mid");
  assert.equal(file.name, "Z song.mid");
  assert.deepEqual(new Uint8Array(await file.arrayBuffer()), midi);
  await assert.rejects(downloadBitMidi({ id: "../escape" }), /Invalid/);

  globalThis.fetch = async () => new Response("<html>blocked</html>");
  await assert.rejects(downloadBitMidi(result.items[0]), /valid MIDI/);
  globalThis.fetch = async () => new Response("", { headers: { "content-length": 5000000 } });
  await assert.rejects(downloadBitMidi(result.items[0]), /too large/);
  globalThis.fetch = async () => new Response("", { status: 429 });
  await assert.rejects(searchBitMidi("test"), /429/);
  globalThis.fetch = async () => { throw new TypeError("Failed to fetch"); };
  await assert.rejects(searchBitMidi("test"), /connection/);
  globalThis.fetch = async () => Response.json({ result: null });
  await assert.rejects(searchBitMidi("test"), /unreadable/);
  const controller = new AbortController();
  controller.abort();
  globalThis.fetch = async (_, options) => { options.signal.throwIfAborted(); };
  await assert.rejects(searchBitMidi("test", 0, controller.signal), { name: "AbortError" });
  console.log("PASS: BitMidi search pagination, sorting, trusted download URLs, MIDI validation, errors and cancellation.");
} finally {
  globalThis.fetch = originalFetch;
}
