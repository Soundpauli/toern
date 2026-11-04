const ORIGIN = "https://bitmidi.com";
const MAX_BYTES = 4 * 1024 * 1024;

async function request(url, signal) {
  const timeout = AbortSignal.timeout(20000);
  try {
    const response = await fetch(url, {
      signal: signal ? AbortSignal.any([signal, timeout]) : timeout,
      credentials: "omit",
    });
    if (!response.ok) throw new Error(`BitMidi returned ${response.status}. Try again shortly.`);
    return response;
  } catch (err) {
    if (signal?.aborted) throw err;
    if (timeout.aborted) throw new Error("BitMidi took too long to respond. Try again.");
    if (err instanceof TypeError) throw new Error("Could not reach BitMidi. Check your connection or import a local MIDI file.");
    throw err;
  }
}

export async function searchBitMidi(query, page = 0, signal) {
  const url = new URL("/api/midi/search", ORIGIN);
  url.search = new URLSearchParams({ q: query.trim(), page: String(page), pageSize: "15" });
  const response = await request(url, signal);
  const { result } = await response.json();
  if (!result || !Array.isArray(result.results)) throw new Error("BitMidi sent an unreadable search response.");
  return {
    total: Number(result.total) || 0,
    pageTotal: Number(result.pageTotal) || 0,
    items: result.results.filter(item => Number.isSafeInteger(item.id) && item.id > 0 && typeof item.name === "string")
      .map(item => ({ id: item.id, name: item.name, plays: Number(item.plays) || 0,
        sourceUrl: `${ORIGIN}/uploads/${item.id}.mid` })),
  };
}

export function sortBitMidi(items, order) {
  const sorted = [...items];
  if (order === "name") sorted.sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true }));
  if (order === "plays") sorted.sort((a, b) => b.plays - a.plays || a.name.localeCompare(b.name));
  return sorted;
}

export async function downloadBitMidi(item, signal) {
  if (!Number.isSafeInteger(item.id) || item.id <= 0) throw new Error("Invalid BitMidi selection.");
  const response = await request(`${ORIGIN}/uploads/${item.id}.mid`, signal);
  if (Number(response.headers.get("content-length")) > MAX_BYTES) throw new Error("This MIDI is too large to import (maximum 4 MB).");
  const bytes = await response.arrayBuffer();
  if (bytes.byteLength > MAX_BYTES) throw new Error("This MIDI is too large to import (maximum 4 MB).");
  if (new Uint8Array(bytes, 0, Math.min(4, bytes.byteLength)).join(",") !== "77,84,104,100") {
    throw new Error("BitMidi did not return a valid MIDI file. Try another result.");
  }
  return new File([bytes], item.name, { type: "audio/midi" });
}
