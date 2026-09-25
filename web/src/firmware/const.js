export const COLS = 32;
export const ROWS = 16;
export const PAGES = 8;
export const STEPS = COLS * PAGES;
export const BPM = 120;
export const STEP_SEC = 60 / BPM / 4;
export const LONG_MS = 500;

export const COL = [
  [0, 0, 0],
  [255, 0, 0],
  [255, 69, 0],
  [255, 255, 0],
  [0, 200, 0],
  [0, 255, 255],
  [0, 0, 255],
  [255, 0, 255],
  [220, 100, 100],
  [0, 0, 0],
  [0, 0, 0],
  [120, 120, 120],
  [0, 0, 0],
  [0, 255, 70],
  [255, 50, 50],
  [40, 40, 40],
];

// colors.h col_base_0. Empty-row / note-off colors. Do not scale these up.
export const COL_BASE = [
  [0, 0, 0],
  [5, 0, 0],
  [18, 4, 0],
  [10, 10, 0],
  [0, 4, 0],
  [0, 5, 5],
  [0, 0, 4],
  [15, 1, 4],
  [22, 12, 16],
  [0, 0, 0],
  [0, 0, 0],
  [6, 6, 6],
  [0, 0, 0],
  [0, 10, 4],
  [18, 4, 4],
];

export const SCHEMES = [
  { col: COL, base: COL_BASE },
  {
    col: [[0, 0, 0], [0, 0, 180], [0, 60, 220], [0, 180, 255], [0, 200, 120], [0, 220, 0], [120, 255, 0], [255, 220, 0], [255, 140, 0], [0, 0, 0], [0, 0, 0], [255, 255, 255], [0, 0, 0], [255, 40, 40], [255, 0, 180]],
    base: [[0, 0, 0], [0, 0, 4], [0, 2, 6], [0, 6, 7], [0, 6, 3], [0, 6, 0], [3, 6, 0], [6, 5, 0], [6, 3, 0], [0, 0, 0], [0, 0, 0], [6, 6, 6], [0, 0, 0], [6, 1, 1], [6, 0, 4]],
  },
  {
    col: [[0, 0, 0], [0, 35, 209], [120, 0, 255], [180, 0, 220], [255, 102, 191], [255, 0, 80], [255, 54, 54], [255, 106, 0], [255, 217, 0], [0, 0, 0], [0, 0, 0], [120, 120, 120], [0, 0, 0], [184, 0, 73], [135, 0, 0]],
    base: [[0, 0, 0], [2, 0, 6], [3, 0, 6], [4, 0, 5], [6, 0, 4], [6, 0, 2], [6, 0, 0], [6, 1, 0], [6, 2, 0], [0, 0, 0], [0, 0, 0], [6, 6, 6], [0, 0, 0], [7, 6, 5], [4, 5, 7]],
  },
];

export const PAINT_ROWS = new Set([2, 3, 4, 5, 6, 7, 8, 9, 12, 14, 15]);
export const SOUND_CH = new Set([1, 2, 3, 4, 5, 6, 7, 8]);
export const FILTER_CH = new Set([1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14]);

export const MENU = ["DAT", "KIT", "WAV", "BPM", "VOL", "SETT", "RECS", "MIDI", "SONG", "ETC"];
export const WAVS = ["KICK", "SNARE", "HAT", "CLAP", "TOM", "RIDE", "PERC", "BASS", "SHKR", "CRSH"];
export const DUMMY_PACKS = {
  1: { name: "DRUM", wavs: [0, 1, 2, 3, 4, 5, 6, 7] },
  2: { name: "HOUSE", wavs: [0, 7, 2, 3, 8, 5, 6, 9] },
};
export const FILTERS = [
  { key: "h", name: "HCUT", letter: "H" },
  { key: "l", name: "LCUT", letter: "L" },
  { key: "r", name: "RVRB", letter: "R" },
  { key: "b", name: "BITC", letter: "B" },
];

export const FILTER_PAGES = [
  [{ key: "h", name: "HCUT" }, { key: "l", name: "LCUT" }, { key: "r", name: "RVRB" }, { key: "b", name: "BITC" }],
  [{ key: "res", name: "RES" }, { key: "detune", name: "DTNE" }, { key: "oct", name: "OCTV" }, null],
  [{ key: "att", name: "ATTC" }, { key: "dec", name: "DCAY" }, { key: "sus", name: "SUST" }, { key: "rel", name: "RLSE" }],
];

export function emptyFilt() {
  return { h: 32, l: 0, r: 0, b: 0, fast: 0, res: 0, detune: 16, oct: 16, att: 32, dec: 32, sus: 10, rel: 5 };
}
export const COND_LABEL = ["1/1", "1/2", "1/4", "1/8", "1/X", "2/1", "4/1", "8/1", "X/1", "F/F", "G/L"];
export const COND_VALUE = [1, 2, 4, 8, 16, 17, 18, 19, 20, 21, 22];
export const PROB = [0, 25, 50, 75, 100];

export function emptyNote() {
  return { channel: 0, velocity: 100, probability: 100, condition: 1, midiPitch: 255 };
}
