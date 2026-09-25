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

function mapf(v, a, b, c, d) {
  return c + ((v - a) / (b - a)) * (d - c);
}

function makeVerb(ctx) {
  const input = ctx.createGain();
  const wet = ctx.createGain();
  wet.gain.value = 1;
  const sum = ctx.createGain();
  sum.gain.value = 0.2;
  const scale = ctx.sampleRate / 44100;
  const combs = [1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617].map((len) => {
    const delay = ctx.createDelay(0.2);
    delay.delayTime.value = (len * scale) / ctx.sampleRate;
    const fb = ctx.createGain();
    fb.gain.value = 0;
    const damp = ctx.createBiquadFilter();
    damp.type = "lowpass";
    damp.frequency.value = 8000;
    input.connect(delay);
    delay.connect(damp);
    damp.connect(fb);
    fb.connect(delay);
    damp.connect(sum);
    return { fb, damp };
  });
  sum.connect(wet);
  return { input, wet, combs };
}

function bitCurve(bits) {
  if (bits >= 16) return null;
  const n = 512;
  const curve = new Float32Array(n);
  const quant = 1 << (16 - bits);
  for (let i = 0; i < n; i++) {
    const x = (i / (n - 1)) * 2 - 1;
    let s = Math.round((x * 32768 + 32768) / quant) * quant - 32768;
    curve[i] = Math.max(-1, Math.min(1, s / 32768));
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
    ctx = new AudioContext({ latencyHint: "interactive" });
    master = ctx.createGain();
    master.gain.value = 0.6;
    master.connect(ctx.destination);
    const previewBus = ctx.createGain();
    previewBus.gain.value = 0.5;
    previewBus.connect(ctx.destination);
    ctx.__preview = previewBus;
    const verb = makeVerb(ctx);
    verb.wet.connect(master);
    KINDS.forEach((kind) => { buffers[kind] = makeBuffer(ctx, kind); });
    for (let ch = 1; ch <= 8; ch++) {
      const input = ctx.createGain();
      const shaper = ctx.createWaveShaper();
      shaper.curve = null;
      const amp = ctx.createGain();
      const hp = ctx.createBiquadFilter();
      hp.type = "highpass";
      hp.frequency.value = 10;
      const lp = ctx.createBiquadFilter();
      lp.type = "lowpass";
      lp.frequency.value = 10000;
      const dry = ctx.createGain();
      const pan = ctx.createStereoPanner();
      const send = ch === 3 || ch === 4 ? null : ctx.createGain();
      input.connect(shaper);
      shaper.connect(amp);
      amp.connect(hp);
      hp.connect(lp);
      lp.connect(dry);
      dry.connect(pan);
      pan.connect(master);
      if (send) {
        send.gain.value = 0;
        lp.connect(send);
        send.connect(verb.input);
      }
      chains[ch] = { input, shaper, amp, hp, lp, dry, pan, send, room: 0 };
    }
    chains.verb = verb;
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
    const q = mapf(filt.res || 0, 0, 32, 0.7, 5);
    if (filt.l > 0) {
      c.hp.frequency.setTargetAtTime(mapf(Math.max(1, filt.l), 1, 32, 40, 2500), now, 0.02);
      c.hp.Q.setTargetAtTime(q, now, 0.02);
      c.lp.frequency.setTargetAtTime(20000, now, 0.02);
      c.lp.Q.setTargetAtTime(0.7, now, 0.02);
    } else {
      c.lp.frequency.setTargetAtTime(mapf(filt.h, 0, 32, 281.25, 10000), now, 0.02);
      c.lp.Q.setTargetAtTime(q, now, 0.02);
      c.hp.frequency.setTargetAtTime(10, now, 0.02);
      c.hp.Q.setTargetAtTime(0.7, now, 0.02);
    }
    const channelvolume = vol / 16;
    const slider = filt.b || 0;
    let amp = channelvolume;
    if (slider <= 0) c.shaper.curve = null;
    else {
      const mv = Math.max(1, Math.min(16, Math.round(mapf(slider, 1, 32, 1, 16))));
      const crushAmt = mapf(mv, 1, 16, 0, 0.8);
      const bits = Math.max(4, Math.min(16, Math.round(mapf(crushAmt, 0, 1, 16, 1))));
      c.shaper.curve = bitCurve(bits);
      amp = mapf(mv, 1, 16, Math.max(channelvolume, 0.1), 0.6);
    }
    c.amp.gain.setTargetAtTime(amp * gain, now, 0.02);
    const hasVerb = !!c.send;
    const verbAmount = hasVerb ? Math.max(0, Math.min(1, mapf(mapf(filt.r || 0, 0, 32, 0, 0.79), 0, 0.79, 0, 1))) : 0;
    const wet = mapf(verbAmount, 0, 1, 0, 0.54);
    const dry = hasVerb ? mapf(verbAmount, 0, 1, 1, 0.88) : 1;
    c.dry.gain.setTargetAtTime(dry, now, 0.02);
    c.room = verbAmount;
    if (c.send) c.send.gain.setTargetAtTime(wet, now, 0.02);
    const tank = chains.verb;
    let room = 0;
    for (let i = 1; i <= 8; i++) room = Math.max(room, chains[i].room || 0);
    const damp = room > 0.01 ? mapf(room, 0, 1, 0.01, 0.42) : 0.25;
    const fb = room <= 0.01 ? 0 : 0.5 + room * 0.35;
    const freq = 14000 * Math.pow(0.02, damp);
    tank.combs.forEach((comb) => {
      comb.fb.gain.setTargetAtTime(fb, now, 0.05);
      comb.damp.frequency.setTargetAtTime(freq, now, 0.05);
    });
  }

  const decoded = new Map();
  function decodeUrl(url) {
    ensure();
    let job = decoded.get(url);
    if (job) {
      decoded.delete(url);
      decoded.set(url, job);
      return job;
    }
    job = fetch(url)
      .then((res) => res.arrayBuffer())
      .then((raw) => ctx.decodeAudioData(raw))
      .catch((err) => { decoded.delete(url); throw err; });
    decoded.set(url, job);
    while (decoded.size > 24) decoded.delete(decoded.keys().next().value);
    return job;
  }

  async function loadUrl(ch, url) {
    custom[ch] = await decodeUrl(url);
  }

  const reversed = new WeakMap();
  const live = new Set();
  const MAX_VOICES = 40;
  const att = (env) => Math.max(0.001, ((32 - env.att) / 32) * 2);
  function play(kind, velocity, when, seek, end, invert, dest, rate = 1, env = null, buffer = null) {
    const srcBuf = buffer || buffers[kind] || buffers.kick;
    let buf = srcBuf;
    if (invert) {
      buf = reversed.get(srcBuf);
      if (!buf) {
        buf = ctx.createBuffer(srcBuf.numberOfChannels, srcBuf.length, srcBuf.sampleRate);
        for (let c = 0; c < srcBuf.numberOfChannels; c++) {
          const s = srcBuf.getChannelData(c);
          const d = buf.getChannelData(c);
          for (let i = 0; i < d.length; i++) d[i] = s[s.length - 1 - i];
        }
        reversed.set(srcBuf, buf);
      }
    }
    const src = ctx.createBufferSource();
    src.buffer = buf;
    src.playbackRate.setValueAtTime(rate, when);
    const span = Math.max(0.01, ((Math.min(100, end) - Math.max(0, seek)) / 100) * buf.duration);
    const offset = (invert ? (100 - Math.min(100, end)) : Math.max(0, seek)) / 100 * buf.duration;
    const g = ctx.createGain();
    const peak = velocity / 127;
    if (!env) g.gain.value = peak;
    else {
      const a = att(env);
      const dec = env.dec / 32;
      const sus = Math.max(0.0001, (env.sus / 32) * peak);
      const rel = Math.max(0.005, env.rel / 32);
      const gate = Math.max(a + dec, a + 0.005);
      g.gain.setValueAtTime(0.0001, when);
      g.gain.linearRampToValueAtTime(peak, when + a);
      g.gain.linearRampToValueAtTime(sus, when + a + dec);
      if (gate > a + dec + 0.0005) g.gain.setValueAtTime(sus, when + gate);
      g.gain.linearRampToValueAtTime(0.0001, when + gate + rel);
    }
    src.connect(g);
    g.connect(dest);
    src.start(when, offset, span / rate);
    if (env) src.stop(when + Math.max(att(env) + env.dec / 32, att(env) + 0.005) + Math.max(0.005, env.rel / 32) + 0.02);
    const voice = { src, g };
    live.add(voice);
    src.onended = () => {
      live.delete(voice);
      src.disconnect();
      g.disconnect();
    };
    if (live.size > MAX_VOICES) {
      const oldest = live.values().next().value;
      live.delete(oldest);
      const t = ctx.currentTime;
      oldest.g.gain.cancelScheduledValues(t);
      oldest.g.gain.setTargetAtTime(0, t, 0.005);
      oldest.src.stop(t + 0.03);
    }
  }

  function trigger(ch, velocity, when, wav, row) {
    if (!SOUND_CH.has(ch) || !chains[ch]) return;
    const home = ch + 1;
    const rate = 2 ** ((((row ?? home) - home) + (wav.detune || 0) + (wav.oct || 0) * 12) / 12);
    play(KINDS[wav.index] || "kick", velocity, when, wav.seek, wav.end, wav.inv, chains[ch].input, rate, wav.env, custom[ch]);
  }

  function preview(wav) {
    ensure();
    if (ctx.state === "suspended") ctx.resume();
    play(KINDS[wav.index] || "kick", wav.velocity ?? 100, ctx.currentTime, wav.seek, wav.end, wav.inv, ctx.__preview, 1, wav.env, wav.buffer || null);
  }

  return { ensure, apply, setBus, loadUrl, decodeUrl, trigger, preview, now: () => (ctx ? ctx.currentTime : 0) };
}
