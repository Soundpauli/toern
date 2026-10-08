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
    for (const ch of [1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14]) {
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
    for (const ch of [1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14]) chains[ch].pan.pan.setTargetAtTime(mix(ch, stereo), ctx.currentTime, 0.02);
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
    if (ch === 11) {
      const base = mapf(filt.cut ?? 16, 0, 32, 220, 7000);
      const open = mapf(filt.h ?? 32, 0, 32, 0.2, 1);
      c.lp.frequency.setTargetAtTime(base * open, now, 0.02);
      c.lp.Q.setTargetAtTime(mapf(Math.max(filt.res || 0, filt.flt || 0), 0, 32, 0.7, 8), now, 0.02);
    }
    if (ch === 13 || ch === 14) setLfo(ch, filt.lfoR || 0, filt.lfoD || 0);
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
  function loadBuffer(ch, buffer) {
    ensure();
    custom[ch] = buffer;
  }

  const reversed = new WeakMap();
  const live = new Set();
  const sampleLive = {};
  const MAX_VOICES = 40;
  const att = (env) => Math.max(0.001, ((32 - env.att) / 32) * 2);
  function stopVoice(voice, when) {
    if (!voice || !live.has(voice)) return;
    live.delete(voice);
    try {
      voice.g.gain.cancelScheduledValues(when);
      voice.g.gain.setTargetAtTime(0.0001, when, 0.008);
      voice.src.stop(when + 0.04);
    } catch { /* already stopped */ }
  }
  function play(kind, velocity, when, seek, end, invert, dest, rate = 1, env = null, buffer = null, stealCh = 0) {
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
    if (stealCh) stopVoice(sampleLive[stealCh], when);
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
    const voice = { src, g, ch: stealCh || 0 };
    live.add(voice);
    if (stealCh) sampleLive[stealCh] = voice;
    src.onended = () => {
      live.delete(voice);
      if (stealCh && sampleLive[stealCh] === voice) delete sampleLive[stealCh];
      src.disconnect();
      g.disconnect();
    };
    if (live.size > MAX_VOICES) {
      const oldest = live.values().next().value;
      live.delete(oldest);
      if (oldest.ch && sampleLive[oldest.ch] === oldest) delete sampleLive[oldest.ch];
      const t = ctx.currentTime;
      oldest.g.gain.cancelScheduledValues(t);
      oldest.g.gain.setTargetAtTime(0, t, 0.005);
      oldest.src.stop(t + 0.03);
    }
  }

  const mono = {};
  const arpAt = {};
  const lfos = {};
  const INST = [
    { type: "sawtooth", a: 0.005, d: 0.12, s: 0.15, r: 0.12 },
    { type: "triangle", a: 0.008, d: 0.18, s: 0.45, r: 0.22 },
    { type: "square", a: 0.002, d: 0.06, s: 0.1, r: 0.05 },
    { type: "sawtooth", a: 0.08, d: 0.4, s: 0.7, r: 0.8 },
    { type: "sine", a: 0.02, d: 0.2, s: 0.35, r: 0.3 },
    { type: "square", a: 0.004, d: 0.05, s: 0.85, r: 0.12 },
    { type: "triangle", a: 0.03, d: 0.15, s: 0.5, r: 0.2 },
    { type: "sawtooth", a: 0.006, d: 0.1, s: 0.55, r: 0.16 },
    { type: "square", a: 0.002, d: 0.08, s: 0.2, r: 0.08 },
    { type: "sawtooth", a: 0.02, d: 0.16, s: 0.6, r: 0.25 },
  ];
  const WAVES = ["sine", "square", "sawtooth", "triangle"];
  function setLfo(ch, rate, depth) {
    const c = chains[ch];
    if (!lfos[ch]) {
      const osc = ctx.createOscillator();
      const g = ctx.createGain();
      osc.type = "sine";
      osc.frequency.value = 0.01;
      g.gain.value = 0;
      osc.connect(g);
      g.connect(c.lp.frequency);
      osc.start();
      lfos[ch] = { osc, g };
    }
    const now = ctx.currentTime;
    lfos[ch].osc.frequency.setTargetAtTime(Math.max(0.05, (rate / 32) * 2), now, 0.03);
    lfos[ch].g.gain.setTargetAtTime(rate > 0 && depth > 0 ? (depth / 32) * 1800 : 0, now, 0.03);
  }
  function synth(ch, velocity, when, row, wav = {}) {
    let midi = Number.isFinite(wav.midiPitch) && wav.midiPitch >= 0 && wav.midiPitch <= 127
      ? wav.midiPitch
      : (ch === 11 ? 36 + (row - 1) : 48 + (row - 1));
    midi += ((wav.cent ?? 16) - 16) / 16 * 24;
    midi += (wav.semi || 0) / 32 * 12;
    midi += wav.detune || 0;
    midi += wav.oct || 0;
    if (ch !== 11 && wav.arp > 0) {
      const semis = (wav.arp / 32) * 12;
      const span = Math.max(2, Math.round(2 + ((wav.span || 0) / 32) * 14));
      const up = span - 1;
      const period = up * 2;
      const step = arpAt[ch] = ((arpAt[ch] || 0) + 1) % period;
      const reflected = step > up ? period - step : step;
      midi += (reflected / up) * semis;
    }
    const freq = 440 * 2 ** ((midi - 69) / 12);
    const preset = INST[Math.max(0, Math.min(9, wav.inst || 0))];
    const osc = ctx.createOscillator();
    osc.type = ch === 11 ? (wav.wave ? WAVES[wav.wave] : preset.type) : (WAVES[wav.wave | 0] || "triangle");
    osc.frequency.setValueAtTime(Math.max(20, freq), when);
    const env = wav.env || {};
    const a = ch === 11 ? preset.a : Math.max(0.005, (env.att ?? 8) / 32 * 0.4);
    const d = ch === 11 ? preset.d : Math.max(0.02, (env.dec ?? 16) / 32 * 0.5);
    const s = ch === 11 ? preset.s : Math.max(0.05, (env.sus ?? 16) / 32);
    const rel = ch === 11 ? preset.r : Math.max(0.02, (env.rel ?? 8) / 32 * 0.6);
    const g = ctx.createGain();
    const peak = Math.max(0.02, (velocity / 127) * (ch === 11 ? 0.22 : 0.28));
    g.gain.setValueAtTime(0.0001, when);
    g.gain.linearRampToValueAtTime(peak, when + a);
    g.gain.linearRampToValueAtTime(peak * s, when + a + d);
    g.gain.exponentialRampToValueAtTime(0.0001, when + a + d + rel);
    osc.connect(g);
    g.connect(chains[ch].input);
    if (ch !== 11 && mono[ch]) {
      mono[ch].g.gain.cancelScheduledValues(when);
      mono[ch].g.gain.setTargetAtTime(0.0001, when, 0.008);
      mono[ch].osc.stop(when + 0.04);
    }
    if (ch !== 11) mono[ch] = { osc, g };
    osc.start(when);
    osc.stop(when + a + d + rel + 0.02);
  }

  function trigger(ch, velocity, when, wav, row) {
    if (!SOUND_CH.has(ch) || !chains[ch]) return;
    if (ch === 11 || ch === 13 || ch === 14) {
      synth(ch, velocity, when, row ?? ch + 1, wav);
      return;
    }
    const home = ch + 1;
    const r = row ?? home;
    // Firmware: midiPitch uses absolute MIDI (root C5=72); else row relative to voice home.
    const semis = Number.isFinite(wav.midiPitch) && wav.midiPitch >= 0 && wav.midiPitch <= 127
      ? (wav.midiPitch - 72)
      : (r - home);
    // wav.detune = fine semis (±1); wav.oct = octave shift (±2) from device.js wavOpts.
    const rate = 2 ** ((semis + (wav.detune || 0) + (wav.oct || 0) * 12) / 12);
    play(KINDS[wav.index] || "kick", velocity, when, wav.seek, wav.end, wav.inv, chains[ch].input, rate, wav.env, custom[ch], ch);
  }

  function preview(wav) {
    ensure();
    if (ctx.state === "suspended") ctx.resume();
    play(KINDS[wav.index] || "kick", wav.velocity ?? 100, ctx.currentTime, wav.seek, wav.end, wav.inv, ctx.__preview, 1, wav.env, wav.buffer || null);
  }

  let clickBuf = null;
  function click() {
    const audio = ensure();
    if (audio.state === "suspended") audio.resume();
    if (!clickBuf) {
      clickBuf = audio.createBuffer(1, 160, audio.sampleRate);
      const data = clickBuf.getChannelData(0);
      for (let i = 0; i < data.length; i++) data[i] = (Math.random() * 2 - 1) * (1 - i / data.length) ** 2;
    }
    const src = audio.createBufferSource();
    src.buffer = clickBuf;
    const hp = audio.createBiquadFilter();
    hp.type = "highpass";
    hp.frequency.value = 1400;
    const gain = audio.createGain();
    gain.gain.value = 0.08;
    src.connect(hp);
    hp.connect(gain);
    gain.connect(audio.destination);
    src.start();
  }

  return { ensure, apply, setBus, loadUrl, loadBuffer, decodeUrl, trigger, preview, click, now: () => (ctx ? ctx.currentTime : 0) };
}
