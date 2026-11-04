(() => {
  /*
    Split-flap display (airport board) for the cover word.
    Adapted from https://jsfiddle.net/kmqe73zd/ / northcoder split-flap-demo:
    top half updates slightly before the bottom half; cells cascade left→right.
  */
  const WORDS = [
    'GALAXY',
    'FALLACY',
    'FANTASY',
    'LUNACY',
    'AUDACITY',
    'VIVACITY',
    'VERACITY',
    'TENACITY',
    'ECSTASY',
  ];

  const CHARACTERS =
    ' ' +
    '1234567890' +
    'ABCDEFGHIJKLMNOPQRSTUVWXYZ' +
    ',.?;:/\'"!&()-’';

  const HOLD_MS = 1000;
  const FLAP_PAUSE = 60;
  const GLYPH_PAUSE = 120;
  const CASCADE = 70;
  const GLYPH_SPLIT_GAP = '1px';

  const flip = document.querySelector('.hb-cover__flip');
  if (!flip) return;

  const reduced = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  const cols = Math.max(...WORDS.map((w) => w.length));
  let index = 0;

  document.documentElement.style.setProperty('--hb-glyph-split-gap', GLYPH_SPLIT_GAP);

  function createCell() {
    const cell = document.createElement('span');
    cell.className = 'hb-cover__cell';
    const top = document.createElement('span');
    top.className = 'hb-cover__glyph hb-cover__glyph--top';
    top.textContent = '\u00A0';
    const bottom = document.createElement('span');
    bottom.className = 'hb-cover__glyph hb-cover__glyph--bottom hb-cover__glyph--gap';
    bottom.textContent = '\u00A0';
    cell.append(top, bottom);
    return cell;
  }

  const line = document.createElement('span');
  line.className = 'hb-cover__line';
  line.setAttribute('aria-hidden', 'true');
  for (let i = 0; i < cols; i++) line.appendChild(createCell());
  flip.replaceChildren(line);

  const live = document.createElement('span');
  live.className = 'hb-vh';
  live.setAttribute('aria-live', 'polite');
  live.textContent = WORDS[0];
  flip.appendChild(live);

  const cells = [...line.children];
  const wait = (ms) => new Promise((resolve) => window.setTimeout(resolve, ms));

  function padWord(word) {
    return word.toUpperCase().padEnd(cols, ' ');
  }

  function setCellInstant(cell, ch) {
    const glyph = ch === ' ' ? '\u00A0' : ch;
    cell.querySelector('.hb-cover__glyph--top').textContent = glyph;
    cell.querySelector('.hb-cover__glyph--bottom').textContent = glyph;
  }

  function paintWord(word) {
    const padded = padWord(word);
    cells.forEach((cell, i) => setCellInstant(cell, padded[i]));
    live.textContent = word.trim();
  }

  async function flapCell(cell, target, delay) {
    await wait(delay);
    const top = cell.querySelector('.hb-cover__glyph--top');
    const bottom = cell.querySelector('.hb-cover__glyph--bottom');
    let cur = top.textContent === '\u00A0' ? ' ' : top.textContent;
    if (cur === target) return;

    const start = CHARACTERS.indexOf(cur);
    const end = CHARACTERS.indexOf(target);
    if (start < 0 || end < 0) {
      setCellInstant(cell, target);
      return;
    }

    // Walk forward through the character belt until the target.
    let i = start;
    while (i !== end) {
      i = (i + 1) % CHARACTERS.length;
      const ch = CHARACTERS[i] === ' ' ? '\u00A0' : CHARACTERS[i];
      top.textContent = ch;
      await wait(FLAP_PAUSE);
      bottom.textContent = ch;
      await wait(GLYPH_PAUSE);
    }
  }

  async function flipTo(word) {
    const padded = padWord(word);
    await Promise.all(
      cells.map((cell, i) => flapCell(cell, padded[i], i * CASCADE)),
    );
    live.textContent = word.trim();
  }

  paintWord(WORDS[0]);

  const hero = document.querySelector('.hb-cover__hero');
  const title = document.querySelector('.hb-cover__title');
  let coverFitTimer = 0;
  function layoutCover() {
    if (!hero || !title) return;
    const stacked = window.matchMedia('(max-width: 720px)').matches;
    hero.style.setProperty('--hb-cover-fit', '1');
    if (stacked) {
      hero.style.removeProperty('--hb-cover-title-h');
      requestAnimationFrame(() => {
        const used = title.scrollWidth;
        const avail = hero.clientWidth;
        if (used > avail + 1 && used > 0) {
          hero.style.setProperty('--hb-cover-fit', String(Math.max(0.42, avail / used)));
        }
      });
      return;
    }
    const gap = parseFloat(getComputedStyle(hero).gap) || 20;
    const matrix = hero.querySelector('.hb-cover__matrix');
    const applyHeight = () => {
      hero.style.setProperty('--hb-cover-title-h', `${Math.max(1, Math.round(title.getBoundingClientRect().height))}px`);
    };
    applyHeight();
    requestAnimationFrame(() => {
      applyHeight();
      const used =
        (matrix ? matrix.getBoundingClientRect().width : 0) +
        title.getBoundingClientRect().width +
        gap;
      const avail = hero.clientWidth;
      if (used > avail + 1 && used > 0) {
        hero.style.setProperty('--hb-cover-fit', String(Math.max(0.42, avail / used)));
        requestAnimationFrame(applyHeight);
      }
    });
  }
  function scheduleCoverLayout() {
    window.clearTimeout(coverFitTimer);
    coverFitTimer = window.setTimeout(layoutCover, 40);
  }
  layoutCover();
  window.addEventListener('resize', scheduleCoverLayout);
  window.addEventListener('hb-layout-change', layoutCover);
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(layoutCover);

  if (reduced) return;

  (async () => {
    for (;;) {
      await wait(HOLD_MS);
      index = (index + 1) % WORDS.length;
      await flipTo(WORDS[index]);
    }
  })();
})();
