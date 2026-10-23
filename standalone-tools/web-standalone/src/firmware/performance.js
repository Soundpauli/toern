// Transport/UI rules shared by the simulator state and matrix renderer.
export function syncDelays(send, receive) {
  return { send: Math.max(0, send) + Math.max(0, -receive), receive: Math.max(0, receive) + Math.max(0, -send) };
}
export function syncHasEffect(s, edit, value) {
  const sends = s.clockInt && s.midiSend !== 1;
  const receives = sends ? s.midiTransport === 1 : !s.analogExtClock;
  return ((edit === 0) !== (value < 0)) ? sends : receives;
}
export function baseRow(ch) { return ch <= 8 ? ch + 1 : ch === 11 ? 12 : ch - 11; }
export function fillHitDue(subtick, length) {
  if (subtick === 0) return false; // Initial hit is explicit.
  const step = Math.floor(subtick / 4);
  const section = Math.min(4, Math.floor(step / Math.ceil(length / 5)));
  const sub = subtick % 4;
  return section === 4 ? sub === 0 || sub === 2 : sub === 0 && step % [8, 4, 2, 1][section] === 0;
}
export function createFillScheduler() {
  const pending = new Set(), active = new Map();
  let clock = 0;
  function arm(ch, cols, grid = null, row = baseRow(ch), velocity = 100, midiPitch = 255) {
    active.set(ch, { start: clock, length: 2 * cols, grid, row, velocity, midiPitch });
  }
  return {
    request(ch) { pending.add(ch); },
    reset() { pending.clear(); active.clear(); clock = 0; },
    // Called once per sequencer step, before grid F/F notes arm their new fills.
    step(step, cols) {
      clock++;
      const hits = [], drops = new Set();
      for (const [ch, f] of active) {
        if (clock - f.start < f.length) continue;
        active.delete(ch);
        if (f.grid) f.grid.condition = 1;
        drops.add(ch);
        hits.push({ ch, row: f.row, velocity: 127, midiPitch: f.midiPitch, sub: 0 });
      }
      if ((step - 1) % cols === 0) {
        for (const ch of pending) {
          arm(ch, cols);
          if (!drops.has(ch)) hits.push({ ch, row: baseRow(ch), velocity: 100, midiPitch: 255, sub: 0 });
        }
        pending.clear();
      }
      for (const [ch, f] of active) {
        const elapsed = (clock - f.start) * 4;
        for (let sub = 0; sub < 4; sub++) if (fillHitDue(elapsed + sub, f.length))
          hits.push({ ch, row: f.row, velocity: f.velocity, midiPitch: f.midiPitch, sub });
      }
      return hits;
    },
    grid(ch, cols, note, row) {
      if (active.has(ch)) return false;
      arm(ch, cols, note, row, note.velocity, note.midiPitch);
      return true;
    },
    owns(ch) { return active.has(ch); },
  };
}
