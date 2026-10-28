// Mirrors src/toern_grid_paint.h. CH11 chords remain audible and protected.
export function canReplaceShadowed(grid, step, row, brush, steps = 256, rows = 16) {
  if (step < 1 || step > steps || row < 1 || row >= rows) return false;
  const blocked = grid[step][row], ch = blocked.channel;
  if (!ch || ch === brush || !((ch >= 1 && ch <= 8) || ch === 13 || ch === 14)) return false;
  const condition = blocked.condition || 1;
  if (condition === 21) return false;
  for (let y = row + 1; y <= rows; ++y) {
    const higher = grid[step][y], higherCondition = higher.condition || 1;
    if (higher.channel === ch && higher.probability >= 100 &&
        (higherCondition === 1 || higherCondition === 22 || higherCondition === condition)) return true;
  }
  return false;
}
