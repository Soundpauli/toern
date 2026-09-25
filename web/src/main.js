import { COL } from "./firmware/const.js";
import { createMatrix } from "./firmware/matrix.js";
import { createDevice } from "./firmware/device.js";

const app = document.querySelector("#app");
const voiceButtons = [1, 2, 3, 4, 5, 6, 7, 8].map((ch) => {
  const [r, g, b] = COL[ch];
  const y = ch + 1;
  const top = ((16 - y + 0.5) / 16) * 100;
  return `<button type="button" class="voice" data-ch="${ch}" style="--c:rgb(${r},${g},${b});--top:${top}%">${ch}</button>`;
}).join("");

app.innerHTML = `
  <div class="device">
    <div class="matrix-wrap">
      <div class="stage">
        <div class="voices">${voiceButtons}</div>
        <div class="matrix-col">
          <canvas id="matrix"></canvas>
          <div class="touch-row">
            <button type="button" class="pad" data-touch="1">L</button>
            <button type="button" class="pad" data-touch="2">R</button>
          </div>
          <div class="encoders">
        <div class="encoder" style="--x:15.625%"><div class="ring"></div><span>Q W · Tab</span></div>
        <div class="encoder" style="--x:37.5%"><div class="ring"></div><span>J K · ⌘</span></div>
        <div class="encoder" style="--x:62.5%"><div class="ring"></div><span>, . · Space</span></div>
        <div class="encoder" style="--x:84.375%"><div class="ring"></div><span>← → · Enter</span></div>
          </div>
          <div class="touch-below">
            <button type="button" class="pad" data-touch="3">3</button>
          </div>
        </div>
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
    while (gesture.acc >= step) {
      device.rotate(i, 1);
      gesture.acc -= step;
      gesture.turned = true;
    }
    while (gesture.acc <= -step) {
      device.rotate(i, -1);
      gesture.acc += step;
      gesture.turned = true;
    }
  });
  encoder.addEventListener("pointerup", () => {
    if (gesture && !gesture.turned) device.press(i);
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
