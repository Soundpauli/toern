import { COL } from "./firmware/const.js";
import { createMatrix } from "./firmware/matrix.js";
import { createDevice } from "./firmware/device.js";

const W = 480;
const H = 440;
const MATRIX = { x: 12, y: 8, w: 456, h: 232 };
const px = (x) => `${(x / W) * 100}%`;
const py = (y) => `${(y / H) * 100}%`;

const app = document.querySelector("#app");
const voiceButtons = [1, 2, 3, 4, 5, 6, 7, 8].map((ch) => {
  const [r, g, b] = COL[ch];
  const y = ch + 1;
  const top = MATRIX.y + (16 - y + 0.5) * (MATRIX.h / 16);
  return `<button type="button" class="voice" data-ch="${ch}" style="--c:rgb(${r},${g},${b});top:${py(top)}">${ch}</button>`;
}).join("");

const ENCODERS = [87, 189, 291, 393];
const encoders = ENCODERS.map((e) => `
  <div class="encoder" style="left:${px(e)};top:${py(352)}">
    <div class="ring"><div class="cap"></div></div>
  </div>`).join("");

const io = (side, kind, pos, label) => {
  const at = side === "front" ? `left:${px(pos)};top:100%` : `top:${py(pos)};${side === "l" ? "left:0" : "left:100%"}`;
  return `<div class="io io--${side} io--${kind}" style="${at}" title="${label}"><span class="io-label">${label}</span></div>`;
};

app.innerHTML = `
  <div class="device">
    <div class="chassis-wrap">
      <div class="chassis">
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
        <div class="hole" style="left:${px(146)};top:${py(266)}"></div>
        <div class="hole" style="left:${px(334)};top:${py(266)}"></div>
        <div class="voices">${voiceButtons}</div>
        <div class="matrix-bed" style="left:${px(MATRIX.x)};top:${py(MATRIX.y)};width:${px(MATRIX.w)};height:${py(MATRIX.h)}">
          <canvas id="matrix"></canvas>
        </div>
        <button type="button" class="pad" data-touch="1" style="left:${px(36)};top:${py(266)}" title="Touch 1"><span>T1</span></button>
        <button type="button" class="pad" data-touch="2" style="left:${px(444)};top:${py(266)}" title="Touch 2"><span>T2</span></button>
        ${encoders}
        <button type="button" class="pad pad--front" data-touch="3" style="left:${px(446)}" title="Touch 3 (record)"><span>T3</span><span class="io-label">Touch 3 (record)</span></button>
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
    <p class="status" id="status"></p>
    <p class="legend">1–8 pick the voice. In single mode the cursor does not change it. Enter paints, hold Enter for velocity, Enter again leaves it. Space plays. F opens the filter, Esc or L leaves it. L toggles single mode off from any row. On WAVE: arrows pick a file and preview it, Q/W and J/K trim, ⌘ inverts, Tab previews, Space loads the file onto the voice.</p>
  </div>
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
  const col = Math.min(32, Math.floor(((e.clientX - rect.left) / rect.width) * 32) + 1);
  const fromTop = Math.min(15, Math.floor(((e.clientY - rect.top) / rect.height) * 16));
  const row = 16 - fromTop;
  return { col, row };
}

canvas.addEventListener("contextmenu", (e) => e.preventDefault());
canvas.addEventListener("pointerdown", (e) => {
  if (e.button !== 0 && e.button !== 2) return;
  drag = e.button === 2;
  canvas.setPointerCapture(e.pointerId);
  const { col, row } = cellFromEvent(e);
  device.pointer(col, row, drag, true);
});
canvas.addEventListener("pointermove", (e) => {
  if (drag === null || (e.buttons === 0)) return;
  const { col, row } = cellFromEvent(e);
  device.pointer(col, row, drag, false);
});
canvas.addEventListener("pointerup", () => { drag = null; });
canvas.addEventListener("pointercancel", () => { drag = null; });

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
document.querySelectorAll(".encoder").forEach((encoder, i) => {
  const ring = encoder.querySelector(".ring");
  let gesture = null;
  encoder.addEventListener("pointerdown", (e) => {
    if (e.button !== 0) return;
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
  encoder.addEventListener("pointerup", () => {
    device.knobUp(i, !!(gesture && gesture.turned));
    gesture = null;
  });
  encoder.addEventListener("pointercancel", () => { gesture = null; });
});

function markVoice() {
  const ch = device.voice();
  document.querySelectorAll(".voice").forEach((btn) => {
    btn.classList.toggle("on", Number(btn.dataset.ch) === ch);
  });
  requestAnimationFrame(markVoice);
}

device.start();
markVoice();
