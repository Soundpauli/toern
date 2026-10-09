/** Cut TŒRN samples from a Demucs stem in the browser (mirrors toern_import.extract_sample). */

export const SAMPLE_RATE = 44100;
export const MAX_SECONDS = 2;
/** Larger files upload fine but have hit SD write errors more often. */
export const WARN_BYTES = 60 * 1024;

const stemCache = new Map();

/** Decode a stem to mono Float32 at 44.1 kHz (OfflineAudioContext keeps that rate). */
export function decodeStem(url) {
  let job = stemCache.get(url);
  if (job) return job;
  job = fetch(url)
    .then((r) => {
      if (!r.ok) throw new Error(`stem: HTTP ${r.status}`);
      return r.arrayBuffer();
    })
    .then((raw) => new OfflineAudioContext(1, 1, SAMPLE_RATE).decodeAudioData(raw))
    .then((buf) => {
      const out = new Float32Array(buf.length);
      for (let c = 0; c < buf.numberOfChannels; c++) {
        const d = buf.getChannelData(c);
        for (let i = 0; i < d.length; i++) out[i] += d[i] / buf.numberOfChannels;
      }
      return { data: out, sr: buf.sampleRate };
    })
    .catch((err) => { stemCache.delete(url); throw err; });
  stemCache.set(url, job);
  return job;
}

/** Same steps as the Python cutter: zero-crossing start, trim silence, short fades, cap peak. */
export function renderCut(stem, start, dur) {
  const { data, sr } = stem;
  dur = Math.min(MAX_SECONDS, Math.max(0.01, dur));
  let a = Math.max(0, Math.round(start * sr));
  const search = Math.round(0.006 * sr);
  let best = -1;
  for (let i = Math.max(1, a - search); i < Math.min(data.length - 1, a + search); i++) {
    if ((data[i] < 0) !== (data[i + 1] < 0) && (best < 0 || Math.abs(i - a) < Math.abs(best - a))) best = i;
  }
  if (best >= 0) a = best;
  const b = Math.min(data.length, a + Math.round(dur * sr));
  let clip = data.slice(a, b);
  let first = 0;
  let last = clip.length - 1;
  while (first < clip.length && Math.abs(clip[first]) <= 2e-4) first++;
  while (last > first && Math.abs(clip[last]) <= 2e-4) last--;
  if (first < clip.length) {
    clip = clip.slice(Math.max(0, first - Math.round(0.003 * sr)), Math.min(clip.length, last + Math.round(0.025 * sr)));
  }
  if (clip.length < 32) clip = new Float32Array(Math.round(0.05 * sr));
  const fi = Math.min(clip.length, Math.round(0.002 * sr));
  const fo = Math.min(clip.length, Math.round(0.012 * sr));
  for (let i = 0; i < fi; i++) clip[i] *= i / fi;
  for (let i = 0; i < fo; i++) clip[clip.length - 1 - i] *= i / fo;
  let peak = 0;
  for (const v of clip) peak = Math.max(peak, Math.abs(v));
  if (peak > 1e-8) {
    const g = Math.min(1, 0.92 / peak);
    for (let i = 0; i < clip.length; i++) clip[i] *= g;
  }
  return clip;
}

/** Canonical 44-byte-header PCM16 mono WAV, as the device expects. */
export function encodeWav(samples, sr = SAMPLE_RATE) {
  const n = samples.length;
  const out = new Uint8Array(44 + n * 2);
  const v = new DataView(out.buffer);
  const tag = (o, t) => { for (let i = 0; i < 4; i++) out[o + i] = t.charCodeAt(i); };
  tag(0, "RIFF"); v.setUint32(4, 36 + n * 2, true); tag(8, "WAVE");
  tag(12, "fmt "); v.setUint32(16, 16, true); v.setUint16(20, 1, true); v.setUint16(22, 1, true);
  v.setUint32(24, sr, true); v.setUint32(28, sr * 2, true); v.setUint16(32, 2, true); v.setUint16(34, 16, true);
  tag(36, "data"); v.setUint32(40, n * 2, true);
  for (let i = 0; i < n; i++) v.setInt16(44 + i * 2, Math.round(Math.max(-1, Math.min(1, samples[i])) * 32767), true);
  return out;
}

export function silentWav() {
  return encodeWav(new Float32Array(1024));
}
