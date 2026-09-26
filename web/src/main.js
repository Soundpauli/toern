import { COL } from "./firmware/const.js";
import { createMatrix } from "./firmware/matrix.js";
import { createDevice } from "./firmware/device.js";

const W = 480;
const H = 440;
const MATRIX = { x: 12, y: 8, w: 456, h: 232 };
const px = (x) => `${(x / W) * 100}%`;
const py = (y) => `${(y / H) * 100}%`;

const app = document.querySelector("#app");
const voiceButtons = [1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14].map((ch) => {
  const [r, g, b] = COL[ch];
  const y = ch + 1;
  const top = MATRIX.y + (16 - y + 0.5) * (MATRIX.h / 16);
  return `<button type="button" class="voice" data-ch="${ch}" style="--c:rgb(${r},${g},${b});top:${py(top)}">${ch}</button>`;
}).join("");

const ENCODERS = [75, 185, 295, 405];
const ENC_KEYS = [[" Q   W ", "TAB"], [" R   T ", "⌘"], [" U   I ", "SPACE"], [" P   Ü ", "ENTER"]];
const encoders = ENCODERS.map((e, i) => `
  <div class="encoder" style="left:${px(e)};top:${py(352)}">
    <div class="ring">
      <div class="cap"></div>
      <span class="enc-keys"><span>&lt;${ENC_KEYS[i][0]}&gt;</span><span class="enc-break"></span><span>↵ ${ENC_KEYS[i][1]}</span></span>
    </div>
  </div>`).join("");

const io = (side, kind, pos, label) => {
  const at = side === "front" ? `left:${px(pos)};top:100%` : `top:${py(pos)};${side === "l" ? "left:0" : "left:100%"}`;
  const yAttr = side === "front" ? "" : ` data-y="${pos}"`;
  return `<div class="io io--${side} io--${kind}" style="${at}"${yAttr} title="${label}"><span class="io-label">${label}</span></div>`;
};

app.innerHTML = `
  <div class="crosshair" aria-hidden="true"><span class="crosshair-x"></span><span class="crosshair-y"></span></div>
  <div class="device">
    <div class="chassis-wrap">
      <div class="chassis">
        <p class="soon" hidden>coming soon</p>
        <div class="panel" style="height:${py(262)}">
          <div class="joints joints--l"></div>
          <div class="joints joints--r"></div>
        </div>
        <div class="box" style="top:${py(248)}">
          <div class="ledge"></div>
          <div class="joints joints--l"></div>
          <div class="joints joints--r"></div>
          <div class="joints joints--b"></div>
          <div class="engrave">TŒRN</div>
        </div>
        <div class="voices">${voiceButtons}</div>
        <div class="matrix-bed" style="left:${px(MATRIX.x)};top:${py(MATRIX.y)};width:${px(MATRIX.w)};height:${py(MATRIX.h)}">
          <canvas id="matrix"></canvas>
        </div>
        <button type="button" class="pad" data-touch="1" style="left:${px(60)};top:${py(266)}" title="Touch 1"><span>T1</span><span class="io-label pad-hint pad-hint--r">Singlemode + Exit</span></button>
        <button type="button" class="pad" data-touch="2" style="left:${px(420)};top:${py(266)}" title="Touch 2"><span>T2</span><span class="io-label pad-hint pad-hint--l">Menu</span></button>
        ${encoders}
        <button type="button" class="pad pad--front" data-touch="3" style="left:${px(420)}" title="Touch 3 (record)"><span>T3</span><span class="io-label">Touch 3 (record)</span></button>
        ${io("l", "jack", 310, "6.35mm line-out (mono)")}
        ${io("l", "usb", 362, "USB-C port")}
        ${io("l", "jack", 414, "6.35mm headphone-out")}
        ${io("r", "switch", 310, "On/off switch")}
        ${io("r", "mini", 362, "3.5mm MIDI-TRS-out")}
        ${io("r", "mini", 414, "3.5mm MIDI-TRS-in")}
        ${io("front", "mini", 60, "PPQN sync port")}
        ${io("front", "mic", 135, "Internal microphone")}
        ${io("front", "jack", 215, "6.35mm mic-input")}
        ${io("front", "jack", 295, "6.35mm line-in (mono)")}
      </div>
    </div>
  </div>
  <aside class="jump">
    <p class="jump-title">Menu</p>
    <div class="jump-types">
      <button type="button" data-device="1">V1</button>
      <button type="button" data-device="2">V2</button>
    </div>
    <button type="button" data-jump="draw">Draw</button>
    <button type="button" data-jump="single">SingleMode</button>
    <button type="button" data-jump="filter">FilterMode</button>
    <button type="button" data-jump="menu">Open MainMenu</button>
    <button type="button" data-jump="wav">LoadSample</button>
    <button type="button" data-jump="bpm">BPM</button>
    <button type="button" id="load-track">load Track</button>
    <div class="jump-sub" id="load-track-sub" hidden>
      <button type="button" data-track="1">Techno</button>
      <button type="button" data-track="2">Hip-hop</button>
      <button type="button" data-track="3">Drum &amp; Bass</button>
      <button type="button" data-track="4">House</button>
      <button type="button" data-track="5">Ambient</button>
    </div>
    <hr>
    <ul class="jump-keys">
      <li><kbd>1</kbd>–<kbd>8</kbd> voice</li>
      <li><kbd>S</kbd> single mode</li>
      <li><kbd>F</kbd> filter</li>
      <li><kbd>L</kbd> load sample</li>
      <li><kbd>D</kbd> clear page</li>
      <li><kbd>Esc</kbd> leave</li>
      <li><kbd>Q</kbd>/<kbd>↑</kbd>, <kbd>W</kbd>/<kbd>↓</kbd> [<kbd>Tab</kbd>] encoder 1</li>
      <li><kbd>R</kbd>, <kbd>T</kbd> [<kbd>⌘</kbd>] encoder 2</li>
      <li><kbd>U</kbd>, <kbd>I</kbd> [<kbd>Space</kbd>] encoder 3</li>
      <li><kbd>P</kbd>/<kbd>←</kbd>, <kbd>Ü</kbd>/<kbd>→</kbd> [<kbd>Enter</kbd>] encoder 4</li>
    </ul>
    <hr>
    <p class="status" id="status"></p>
    <p class="jump-help" id="jump-help"></p>
    <label class="jump-led"><span>LED</span><input type="checkbox" id="led-style" checked></label>
  </aside>
`;

const matrix = createMatrix(document.querySelector("#matrix"));
const device = createDevice(
  matrix,
  document.querySelector("#status"),
  [...document.querySelectorAll(".ring")]
);
window.addEventListener("keydown", (e) => device.keydown(e), true);
window.addEventListener("keyup", (e) => device.keyup(e), true);

const canvas = document.querySelector("#matrix");
let drag = null;

function cellFromEvent(e) {
  const rect = canvas.getBoundingClientRect();
  const cols = device.cols();
  const col = Math.min(cols, Math.floor(((e.clientX - rect.left) / rect.width) * cols) + 1);
  const fromTop = Math.min(15, Math.floor(((e.clientY - rect.top) / rect.height) * 16));
  const row = 16 - fromTop;
  return { col, row };
}

canvas.addEventListener("contextmenu", (e) => e.preventDefault());
let press = null;
function endPress(unpaint) {
  if (!press) return;
  clearTimeout(press.timer);
  if (unpaint && press.had && !press.moved && !press.long) device.pointer(press.col, press.row, true, false);
  press = null;
}
canvas.addEventListener("pointerdown", (e) => {
  if (e.button === 2) {
    drag = true;
    canvas.setPointerCapture(e.pointerId);
    const { col, row } = cellFromEvent(e);
    device.pointer(col, row, true, false);
    return;
  }
  if (e.button !== 0) return;
  const { col, row } = cellFromEvent(e);
  const had = device.cellTaken(col, row);
  press = { col, row, had, moved: false, long: false, timer: 0 };
  drag = false;
  canvas.setPointerCapture(e.pointerId);
  if (!had) device.pointer(col, row, false, true);
  else press.timer = setTimeout(() => {
    if (!press || press.moved) return;
    press.long = true;
    device.hover(press.col, press.row);
    device.velocity();
  }, 500);
});
canvas.addEventListener("pointermove", (e) => {
  const { col, row } = cellFromEvent(e);
  if (drag === null) {
    device.hover(col, row);
    return;
  }
  if (e.buttons === 0) return;
  if (press && (col !== press.col || row !== press.row)) {
    press.moved = true;
    clearTimeout(press.timer);
  }
  device.pointer(col, row, drag === true, false);
});
canvas.addEventListener("pointerup", () => { endPress(true); drag = null; });
canvas.addEventListener("pointercancel", () => { endPress(false); drag = null; });

document.querySelectorAll(".pad").forEach((pad) => {
  const id = Number(pad.dataset.touch);
  pad.addEventListener("pointerdown", (e) => {
    if (e.button !== 0) return;
    pad.classList.add("on");
    pad.setPointerCapture(e.pointerId);
    device.touch(id, true);
  });
  const release = () => {
    pad.classList.remove("on");
    device.touch(id, false);
  };
  pad.addEventListener("pointerup", release);
  pad.addEventListener("pointercancel", release);
});
document.querySelectorAll(".voice").forEach((btn) => {
  btn.addEventListener("click", () => device.selectVoice(Number(btn.dataset.ch)));
});
document.querySelectorAll(".jump-types button").forEach((btn) => {
  btn.addEventListener("click", () => device.setDevice(Number(btn.dataset.device)));
});
const LED_PRESET = {
  1: { boost: 0, size: 29, diffuse: 11, blur: 49, base: 0, capOn: 46, capOff: 15, capOpacity: 100 },
  2: { boost: 109, size: 45, diffuse: 23, blur: 59, base: 0, capOn: 61, capOff: 26, capOpacity: 100 },
};
function applyLedPreset(gen) {
  matrix.setLook(LED_PRESET[gen === 1 ? 1 : 2]);
}
const ledStyle = document.querySelector("#led-style");
ledStyle.checked = localStorage.getItem("toern-web-led") !== "0";
if (ledStyle.checked) matrix.setPhoto(true);
ledStyle.addEventListener("change", () => {
  localStorage.setItem("toern-web-led", ledStyle.checked ? "1" : "0");
  matrix.setPhoto(ledStyle.checked);
});
document.querySelectorAll(".encoder").forEach((encoder, i) => {
  const ring = encoder.querySelector(".ring");
  let gesture = null;
  encoder.addEventListener("pointerdown", (e) => {
    if (e.button !== 0) return;
    encoder.classList.add("pressed");
    device.knobDown(i);
    const rect = ring.getBoundingClientRect();
    const cx = rect.left + rect.width / 2;
    const cy = rect.top + rect.height / 2;
    gesture = {
      cx, cy, x: e.clientX, y: e.clientY,
      last: Math.atan2(e.clientY - cy, e.clientX - cx),
      acc: 0,
      turned: false,
    };
    encoder.setPointerCapture(e.pointerId);
  });
  encoder.addEventListener("pointermove", (e) => {
    if (!gesture) return;
    const moved = Math.hypot(e.clientX - gesture.x, e.clientY - gesture.y);
    const radius = Math.hypot(e.clientX - gesture.cx, e.clientY - gesture.cy);
    if (moved < 8 || radius < 12) return;
    const ang = Math.atan2(e.clientY - gesture.cy, e.clientX - gesture.cx);
    let delta = ang - gesture.last;
    if (delta > Math.PI) delta -= Math.PI * 2;
    if (delta < -Math.PI) delta += Math.PI * 2;
    gesture.last = ang;
    gesture.acc += delta;
    const step = 0.35;
    const sense = i === 0 && device.gridMode() ? -1 : 1;
    while (gesture.acc >= step) {
      device.rotate(i, sense);
      gesture.acc -= step;
      gesture.turned = true;
    }
    while (gesture.acc <= -step) {
      device.rotate(i, -sense);
      gesture.acc += step;
      gesture.turned = true;
    }
  });
  const release = () => {
    encoder.classList.remove("pressed");
    device.knobUp(i, !!(gesture && gesture.turned));
    gesture = null;
  };
  encoder.addEventListener("pointerup", release);
  encoder.addEventListener("pointercancel", () => {
    encoder.classList.remove("pressed");
    gesture = null;
  });
});

const encodersEl = [...document.querySelectorAll(".encoder")];
function pressKey(e) {
  if (e.repeat) return;
  if (e.code === "Tab" || e.key === "Tab") return 0;
  if (e.key === "Meta" || e.code === "MetaLeft" || e.code === "MetaRight") return 1;
  if (e.key === " ") return 2;
  if (e.key === "Enter") return 3;
  return -1;
}
function touchKey(e) {
  if (e.key === "ß" || e.key === "ẞ" || e.key === "-" || e.code === "Minus") return 1;
  if (e.key === "´" || e.code === "Equal") return 2;
  return 0;
}
window.addEventListener("keydown", (e) => {
  const i = pressKey(e);
  if (i >= 0) encodersEl[i].classList.add("pressed");
  const touch = touchKey(e);
  if (touch && !e.repeat) document.querySelector(`.pad[data-touch="${touch}"]`)?.classList.add("on");
}, true);
window.addEventListener("keyup", (e) => {
  const i = pressKey(e);
  if (i >= 0) encodersEl[i].classList.remove("pressed");
  const touch = touchKey(e);
  if (touch) document.querySelector(`.pad[data-touch="${touch}"]`)?.classList.remove("on");
}, true);

document.querySelectorAll(".jump button").forEach((btn) => {
  btn.addEventListener("click", () => device.jump(btn.dataset.jump));
});
const loadTrack = document.querySelector("#load-track");
const loadTrackSub = document.querySelector("#load-track-sub");
loadTrack.addEventListener("click", () => {
  loadTrackSub.hidden = !loadTrackSub.hidden;
  loadTrack.classList.toggle("on", !loadTrackSub.hidden);
});
loadTrackSub.querySelectorAll("button").forEach((btn) => {
  btn.addEventListener("click", () => device.loadGenre(Number(btn.dataset.track)));
});

const HELP = {
  draw: "Draw is the grid. Move with the encoders, Enter paints a note, Space plays. D clears this page. T1 enters single mode on the current row. T2 opens the main menu.",
  single: "Single mode is one voice, and each row is a pitch. Enter paints that voice. On the bottom row, hold Tab to generate a pattern. D clears only this voice. T1 returns to draw.",
  filter: "Filter shapes the current voice. The four encoders are the controls on this page. T2 moves to the next filter page. T1 leaves. T3 resets this page.",
  menu: "Main menu. Encoder 4 moves through the pages, Enter opens the one you are on. T2 or Esc leaves.",
  wav: "Load a sample onto this voice. Encoder 4 moves through folders and files, Enter opens a folder or loads the file. Q/W and R/T trim the sample, Tab previews it. T3 records from the mic.",
  rec: "Mic record. Encoder 2 starts the take, encoder 3 stops it or plays it back, encoder 4 leaves. The recording replaces this voice.",
  bpm: "Tempo. Encoder 4 sets the BPM. Tap T3 to set the tempo by hand.",
  velocity: "Velocity of the note under the cursor: encoder 1 is level, 2 is probability, 3 is condition, 4 is voice volume. Enter leaves.",
  dat: "Save or load a pattern. Encoder 4 picks the slot. Encoder 1 loads it, encoder 2 saves it. An empty slot opens NEW.",
  new: "NEW writes a pattern. Encoder 3 picks BLNK, TECH, HIPH, DNB, HOUS, or AMBT. Encoder 4 sets how many pages, except for BLNK. Enter writes it. Encoder 1 goes back to the file.",
  pat: "Load a sample pattern. Encoder 3 picks techno, hip-hop, drum and bass, house, or ambient. Encoder 4 sets how many pages. Enter writes it.",
  kit: "Load a sound pack onto the eight voices. Encoder 4 picks the pack, encoder 2 loads it.",
  song: "Song arranges patterns in order. Encoder 4 moves through the song, encoder 2 picks the pattern. Encoder 3 starts the song.",
  look: "Settings. Encoder 4 picks a row, encoder 3 changes it. LEDS 1 and 1B use a 16×16 matrix, 2 and 2B use 32×16. Esc leaves.",
  vol: "Output levels. Encoder 4 picks main, gain, or preview, encoder 3 changes it.",
  recs: "Recording input. Encoder 4 picks a row, encoder 3 changes it.",
  midi: "MIDI. Encoder 4 picks a row, encoder 3 changes it.",
  etc: "Info, storage, and reset. Encoder 4 picks a row. Esc leaves.",
  shift: "Shift moves the current voice. Encoder 4 moves it in time, encoder 1 moves it in pitch, encoder 2 moves only this page. Esc leaves.",
  boot: "The device is starting.",
};

const helpEl = document.querySelector("#jump-help");
let helpMode = "";
let ledLookGen = 0;
const crosshair = document.querySelector(".crosshair");
function markVoice() {
  const ch = device.voice();
  document.querySelectorAll(".voice").forEach((btn) => {
    const n = Number(btn.dataset.ch);
    btn.classList.toggle("on", n === ch);
    btn.classList.toggle("muted", device.muted(n));
  });
  const mode = device.mode();
  document.querySelector(".soon").hidden = !device.soon();
  document.querySelector(".voices").classList.toggle("on", mode === "draw" || mode === "single" || mode === "filter");
  document.querySelectorAll(".jump button[data-jump]").forEach((btn) => {
    btn.classList.toggle("on", btn.dataset.jump === mode);
  });
  if (mode !== helpMode) {
    helpMode = mode;
    helpEl.textContent = HELP[mode] || "Esc leaves this screen.";
  }
  const cols = device.cols();
  const wide = cols > 16;
  const ledGen = wide ? 2 : 1;
  if (ledGen !== ledLookGen) {
    ledLookGen = ledGen;
    applyLedPreset(ledGen);
  }
  const chassisH = wide ? 440 : 658;
  const boxTop = wide ? 248 : 466;
  const shift = boxTop - 248;
  const pyNow = (y) => `${(y / chassisH) * 100}%`;
  const chassis = document.querySelector(".chassis");
  chassis.style.aspectRatio = `480 / ${chassisH}`;
  document.querySelector(".panel").style.height = pyNow(wide ? 262 : 480);
  document.querySelector(".box").style.top = pyNow(boxTop);
  const bed = document.querySelector(".matrix-bed");
  const matrixBox = wide ? { x: 12, y: 8, w: 456, h: 232 } : { x: 12, y: 12, w: 456, h: 456 };
  bed.style.left = px(matrixBox.x);
  bed.style.top = pyNow(matrixBox.y);
  bed.style.width = px(matrixBox.w);
  bed.style.height = pyNow(matrixBox.h);
  document.querySelectorAll(".encoder").forEach((el, i) => {
    el.style.top = pyNow(352 + shift);
    el.style.left = px([75, 185, 295, 405][i]);
  });
  document.querySelector('.pad[data-touch="1"]').style.top = pyNow(266 + shift);
  document.querySelector('.pad[data-touch="2"]').style.top = pyNow(266 + shift);
  document.querySelectorAll(".io--l, .io--r").forEach((el) => {
    const base = Number(el.dataset.y);
    el.style.top = pyNow(base + shift);
  });
  document.querySelectorAll(".voice").forEach((btn) => {
    const y = Number(btn.dataset.ch) + 1;
    const top = matrixBox.y + (16 - y + 0.5) * (matrixBox.h / 16);
    btn.style.top = pyNow(top);
  });
  document.querySelectorAll(".jump-types button").forEach((btn) => {
    btn.classList.toggle("on", Number(btn.dataset.device) === (wide ? 2 : 1));
  });
  const cur = device.cursor();
  crosshair.classList.toggle("on", cur.on);
  if (cur.on) {
    const rect = canvas.getBoundingClientRect();
    const cx = rect.left + ((cur.x - 0.5) / cols) * rect.width;
    const cy = rect.top + ((16 - cur.y + 0.5) / 16) * rect.height;
    document.documentElement.style.setProperty("--cx", `${cx}px`);
    document.documentElement.style.setProperty("--cy", `${cy}px`);
  }
  requestAnimationFrame(markVoice);
}

device.start();
markVoice();
