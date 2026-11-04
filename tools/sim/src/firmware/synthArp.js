// Mirrors arpSigned/arpSpanSteps/midiArpFireChannel in toern.ino.
export function arpSettings(params = {}) {
  const signed = Math.max(0, Math.min(32, params.arp ?? 16)) - 16;
  return { enabled: signed !== 0, eighths: signed < 0,
    semitones: 1 + (Math.abs(signed) - 1) * 11 / 15,
    span: Math.round(2 + Math.max(0, Math.min(32, params.span ?? 0)) * 14 / 32) };
}
export function createSynthArp({ params, play, release }) {
  const voices = new Map();
  function voice(ch) {
    if (!voices.has(ch)) voices.set(ch, { held: new Map(), step: 0, lastAt: 0, lastBeat: null, enabled: false });
    return voices.get(ch);
  }
  function fire(ch, v, cfg, clock) {
    const held = [...v.held.values()].sort((a,b) => a.pitch - b.pitch);
    if (!held.length) return;
    const peak = cfg.span - 1, period = peak * 2;
    const phase = v.step++ % period, index = phase > peak ? period - phase : phase;
    const note = held.length > 1 ? held[index % held.length] : held[0];
    const pitch = held.length > 1 ? note.pitch + 12 * Math.floor(index / held.length)
      : note.pitch + Math.round(index * cfg.semitones);
    play(ch, Math.max(0, Math.min(127, pitch)), note.velocity, clock.when, false);
    v.lastAt = clock.now;
    v.lastBeat = cfg.eighths ? Math.floor((clock.beat + 1) / 2) : clock.beat;
  }
  return {
    noteOn(ch, key, pitch, velocity, clock) {
      const v = voice(ch), first = v.held.size === 0;
      v.held.set(key, { pitch, velocity });
      const cfg = arpSettings(params(ch));
      v.enabled = cfg.enabled;
      if (!cfg.enabled) { v.step = 0; play(ch, pitch, velocity, clock.when, true); }
      else if (first) { v.step = 0; fire(ch, v, cfg, clock); }
    },
    // Match the original input key even if the selected TŒRN voice changed.
    noteOff(key) {
      for (const [ch,v] of voices) {
        const note = v.held.get(key);
        if (!note) continue;
        v.held.delete(key);
        if (!v.held.size) { v.step = 0; v.lastBeat = null; release(ch); }
        else if (!arpSettings(params(ch)).enabled) release(ch, note.pitch);
      }
    },
    hasHeld(ch) { return !!voices.get(ch)?.held.size; },
    service(clock) {
      for (const [ch,v] of voices) {
        if (!v.held.size) continue;
        const cfg = arpSettings(params(ch));
        if (!cfg.enabled) { v.step = 0; v.enabled = false; continue; }
        if (!v.enabled) { v.enabled = true; v.step = 0; fire(ch,v,cfg,clock); continue; }
        if (clock.playing) continue; // Playing advances only from the playhead.
        const interval = Math.max(20, Math.round(60000 / (clock.bpm * (cfg.eighths ? 2 : 4))));
        if (clock.now - v.lastAt >= interval) fire(ch,v,cfg,clock);
      }
    },
    playhead(clock) {
      for (const [ch,v] of voices) {
        const cfg = arpSettings(params(ch));
        if (!v.held.size || !cfg.enabled || (cfg.eighths && clock.beat % 2 === 0)) continue;
        const beat = cfg.eighths ? Math.floor((clock.beat + 1) / 2) : clock.beat;
        if (beat !== v.lastBeat) fire(ch,v,cfg,clock);
      }
    },
    reset() {
      for (const [ch,v] of voices) { if (v.held.size) release(ch); }
      voices.clear();
    },
  };
}
