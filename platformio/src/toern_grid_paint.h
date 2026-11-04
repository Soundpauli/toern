#pragma once

namespace toern_grid_paint {
// Rows play in ascending order; only mono voices lose their lower-row hit.
// A probabilistic/conditional winner must cover every possible lower hit.
template <typename Grid>
bool canReplaceShadowed(const Grid &grid, unsigned step, unsigned row,
                        unsigned brush, unsigned steps, unsigned rows) {
  if (step < 1 || step > steps || row < 1 || row >= rows) return false;
  const auto &blocked = grid[step][row];
  const unsigned ch = blocked.channel;
  if (!ch || ch == brush || !((ch >= 1 && ch <= 8) || ch == 13 || ch == 14)) return false;
  const unsigned condition = blocked.condition ? blocked.condition : 1;
  if (condition == 21) return false; // Fills have their own playback timer.
  for (unsigned y = row + 1; y <= rows; ++y) {
    const auto &higher = grid[step][y];
    if (higher.channel != ch || higher.probability < 100) continue;
    const unsigned higherCondition = higher.condition ? higher.condition : 1;
    if (higherCondition == 1 || higherCondition == 22 || higherCondition == condition) return true;
  }
  return false;
}
}
