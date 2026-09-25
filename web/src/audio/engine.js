import { SOUND_CH } from "../firmware/const.js";

const KINDS = ["kick", "snare", "hat", "clap", "tom", "ride", "perc", "bass", "shaker", "crash"];

function makeBuffer(ctx, kind) {
  const sr = ctx.sampleRate;
  const dur = kind === "hat" ? 0.05 : kind === "ride" ? 0.4 : 0.26;
  const len = Math.floor(sr * dur);
  const buf = ctx.createBuffer(1, len, sr);
  const d = buf.getChannelData(0);
  for (let i = 0; i < len; i++) {
    const t = i / sr;
    if (kind === "kick") d[i] = Math.sin(2 * Math.PI * 150 * Math.pow(0.2, t / 0.22) * t) * Math.exp(-t * 9);
    else if (kind === "snare") d[i] = ((Math.random() * 2 - 1) * 0.55 + Math.sin(2 * Math.PI * 190 * t) * 0.45) * Math.exp(-t * 11);
    else if (kind === "hat") d[i] = (Math.random() * 2 - 1) * Math.exp(-t * 55);
    else if (kind === "clap") d[i] = (Math.random() * 2 - 1) * (t < 0.012 || t > 0.03 ? 1 : 0.25) * Math.exp(-t * 13);
    else if (kind === "tom") d[i] = Math.sin(2 * Math.PI * 120 * Math.pow(0.45, t) * t) * Math.exp(-t * 7);
    else if (kind === "ride") d[i] = (Math.random() * 2 - 1) * Math.exp(-t * 3.5) * 0.45;
    else if (kind === "perc") d[i] = Math.sin(2 * Math.PI * 700 * t) * Math.exp(-t * 28);
    else if (kind === "shaker") d[i] = (Math.random() * 2 - 1) * Math.exp(-t * 18) * (0.3 + 0.7 * (Math.sin(t * 180) > 0 ? 1 : 0.2));
    else if (kind === "crash") d[i] = (Math.random() * 2 - 1) * Math.exp(-t * 2.2) * 0.7;
    else d[i] = Math.sin(2 * Math.PI * 52 * t) * Math.exp(-t * 5);
  }
  return buf;
}

function crushCurve(amount) {
  const n = 256;
  const curve = new Float32Array(n);
  const steps = Math.max(2, Math.round(72 - amount * 2));
  for (let i = 0; i < n; i++) {
    const x = (i / (n - 1)) * 2 - 1;
    curve[i] = Math.round(x * steps) / steps;
  }
  return curve;
}

export function createEngine() {
  let ctx = null;
  let master = null;
  const chains = {};
  const buffers = {};
  const custom = {};

  function ensure() {
    if (ctx) return ctx;
    ctx = new AudioContext();
    master = ctx.createGain();
    master.gain.value = 0.6;
    master.connect(ctx.destination);
    const previewBus = ctx.createGain();
    previewBus.gain.value = 0.5;
    previewBus.connect(ctx.destination);
    ctx.__preview = previewBus;
    const rev = ctx.createGain();
    const delay = ctx.createDelay();
    delay.delayTime.value = 0.045;
    const fb = ctx.createGain();
    fb.gain.value = 0.42;
    const damp = ctx.createBiquadFilter();
    damp.type = "lowpass";
    damp.frequency.value = 3200;
    rev.connect(delay);
    delay.connect(damp);
    damp.connect(fb);
    fb.connect(delay);
    delay.connect(master);
    KINDS.forEach((kind) => { buffers[kind] = makeBuffer(ctx, kind); });
    for (let ch = 1; ch <= 8; ch++) {
      const hp = ctx.createBiquadFilter();
      hp.type = "highpass";
      const lp = ctx.createBiquadFilter();
      lp.type = "lowpass";
      const shaper = ctx.createWaveShaper();
      const out = ctx.createGain();
      const send = ctx.createGain();
      const pan = ctx.createStereoPanner();
      hp.connect(lp);
      lp.connect(shaper);
      shaper.connect(out);
      out.connect(pan);
      pan.connect(master);
      shaper.connect(send);
      send.connect(rev);
      chains[ch] = { hp, lp, shaper, out, send, pan };
    }
    return ctx;
  }

  function mix(ch, stereo) {
    if (!stereo) return 0;
    if (stereo === 1) return ch <= 4 ? -0.35 : 0.35;
    return ch % 2 ? -0.75 : 0.75;
  }

  function setBus({ main = 80, preview = 8, stereo = 0 } = {}) {
    if (!ctx) return;
    master.gain.setTargetAtTime((main / 100) * 0.75, ctx.currentTime, 0.02);
    ctx.__preview.gain.setTargetAtTime(preview / 16, ctx.currentTime, 0.02);
    for (let ch = 1; ch <= 8; ch++) chains[ch].pan.pan.setTargetAtTime(mix(ch, stereo), ctx.currentTime, 0.02);
  }

  function apply(ch, filt, vol, gain = 1) {
    if (!ctx || !chains[ch]) return;
    const c = chains[ch];
    const now = ctx.currentTime;
    const h = 280 * Math.pow(10000 / 280, filt.h / 32);
    const l = filt.l <= 0 ? 20 : 40 * Math.pow(2500 / 40, (filt.l - 1) / 31);
    c.lp.frequency.setTargetAtTime(h, now, 0.02);
    c.hp.frequency.setTargetAtTime(l, now, 0.02);
    c.send.gain.setTargetAtTime((filt.r / 32) * 0.65, now, 0.02);
    c.shaper.curve = filt.b <= 0 ? null : crushCurve(filt.b);
    c.lp.Q.setTargetAtTime(0.7 + (filt.res || 0) * 0.6, now, 0.02);
    c.out.gain.setTargetAtTime((vol / 16) * gain, now, 0.02);
  }

  async function loadUrl(ch, url) {
    ensure();
    const raw = await (await fetch(url)).arrayBuffer();
    custom[ch] = await ctx.decodeAudioData(raw.slice(0));
  }

  function clip(kind, seek, end, invert, srcBuf) {
    srcBuf = srcBuf || buffers[kind] || buffers.kick;
    const data = srcBuf.getChannelData(0);
    const a = Math.floor((Math.max(0, seek) / 100) * data.length);
    const b = Math.max(a + 1, Math.floor((Math.min(100, end) / 100) * data.length));
    const out = ctx.createBuffer(1, b - a, ctx.sampleRate);
    const dst = out.getChannelData(0);
    for (let i = 0; i < dst.length; i++) dst[i] = data[invert ? b - 1 - i : a + i];
    return out;
  }

  function play(kind, velocity, when, seek, end, invert, dest, rate = 1, env = null, buffer = null) {
    const src = ctx.createBufferSource();
    src.buffer = clip(kind, seek, end, invert, buffer);
    src.playbackRate.setValueAtTime(rate, when);
    const g = ctx.createGain();
    const peak = velocity / 127;
    if (!env) g.gain.value = peak;
    else {
      const dur = Math.max(0.05, src.buffer.duration / rate);
      const att = (env.att / 32) * 0.25;
      const dec = (env.dec / 32) * 0.4;
      const sus = env.sus / 32;
      const rel = Math.max(0.02, (env.rel / 32) * 0.5);
      g.gain.setValueAtTime(0.0001, when);
      g.gain.linearRampToValueAtTime(peak, when + att);
      g.gain.linearRampToValueAtTime(Math.max(0.0001, peak * sus), when + att + dec);
      g.gain.setValueAtTime(Math.max(0.0001, peak * sus), when + Math.max(att + dec, dur - rel));
      g.gain.linearRampToValueAtTime(0.0001, when + dur);
    }
    src.connect(g);
    g.connect(dest);
    src.start(when);
  }

  function trigger(ch, velocity, when, wav, row) {
    if (!SOUND_CH.has(ch) || !chains[ch]) return;
    const home = ch + 1;
    const rate = 2 ** ((((row ?? home) - home) + (wav.detune || 0) + (wav.oct || 0) * 12) / 12);
    play(KINDS[wav.index] || "kick", velocity, when, wav.seek, wav.end, wav.inv, chains[ch].hp, rate, wav.env, custom[ch]);
  }

  function preview(wav) {
    ensure();
    if (ctx.state === "suspended") ctx.resume();
    play(KINDS[wav.index] || "kick", wav.velocity ?? 100, ctx.currentTime, wav.seek, wav.end, wav.inv, ctx.__preview, 1, wav.env, wav.buffer || null);
  }

  return { ensure, apply, setBus, loadUrl, trigger, preview, now: () => (ctx ? ctx.currentTime : 0) };
}
