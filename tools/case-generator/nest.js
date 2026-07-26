/** Simple grid nest layout for panels */

import { panelBounds } from "./unfold.js";

/**
 * Place panels in rows with margin.
 * @returns {{ items: NestItem[], width, height }}
 * NestItem: { panel, x, y, bounds }
 */
export function nestPanels(panels, { margin = 8, maxRowWidth = 400 } = {}) {
  const items = [];
  let x = margin;
  let y = margin;
  let rowH = 0;
  let maxX = margin;
  let maxY = margin;

  for (const panel of panels) {
    const bounds = panelBounds(panel);
    const w = bounds.width;
    const h = bounds.height;

    if (x > margin && x + w + margin > maxRowWidth) {
      x = margin;
      y += rowH + margin;
      rowH = 0;
    }

    items.push({
      panel,
      x,
      y,
      bounds,
    });

    maxX = Math.max(maxX, x + w + margin);
    maxY = Math.max(maxY, y + h + margin);
    x += w + margin;
    rowH = Math.max(rowH, h);
  }

  return {
    items,
    width: Math.max(maxX, margin * 2),
    height: Math.max(maxY, margin * 2),
    margin,
  };
}
