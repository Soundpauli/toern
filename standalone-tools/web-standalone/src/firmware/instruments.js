// Control defaults mirrored from toern_parameters.ino; browser synthesis is approximate.
export const INSTRUMENT_NAMES = ["BASS", "KEYS", "CHPT", "PAD", "WOW", "ORG", "FLT", "LEAD", "ARP", "BRSS", "303", "DXEP", "DXBS"];
const tables = {"cut": [8, 20, 12, 18, 5, 24, 20, 22, 26, 9, 14, 20, 14], "res": [5, 4, 2, 10, 24, 1, 22, 14, 6, 10, 12, 6, 10], "flt": [12, 8, 2, 3, 20, 2, 2, 14, 4, 18, 12, 10, 12], "att": [32, 32, 32, 8, 26, 32, 28, 32, 32, 30, 32, 32, 32], "dec": [6, 14, 1, 32, 32, 2, 0, 5, 0, 28, 18, 28, 16], "sus": [0, 2, 28, 20, 26, 28, 31, 0, 31, 26, 18, 20, 16], "rel": [2, 6, 2, 24, 20, 3, 10, 5, 1, 10, 12, 16, 12], "cent": [5, 14, 14, 14, 14, 18, 18, 14, 18, 14, 5, 14, 5]};
export function instrumentDefaults(inst) {
  const result = { inst, semi: 16, wave: [2,1,1,0,3,0,3,1,1,2,2,3,1][inst] };
  for (const [key, values] of Object.entries(tables)) result[key] = values[inst];
  return result;
}
