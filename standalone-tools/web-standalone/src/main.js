import { COL } from "./firmware/const.js";
import { createMatrix } from "./firmware/matrix.js";
import { createDevice } from "./firmware/device.js";
import { parseMidiFile, mapNotesToGrid, filterOverlappingNotes, buildImportCells } from "./firmware/midiImport.js";
import { ToernSdSerial, joinSdPath } from "./firmware/sdSerial.js";
import { encodePatternRam, decodePatternFile } from "./firmware/patternFile.js";

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
  return `<button type="button" class="voice" data-ch="${ch}" aria-label="Voice ${ch}" aria-pressed="false" style="--c:rgb(${r},${g},${b});top:${py(top)}">${ch}<span class="voice-mute" aria-hidden="true"></span></button>`;
}).join("");

const ENCODERS = [75, 185, 295, 405];
const ENC_KEYS = [[" Q   W ", "TAB"], [" R   T ", "⌥/⇧"], [" U   I ", "SPACE"], [" P   Ü ", "ENTER"]];
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
  <div class="entry-gate" id="entry-gate" role="dialog" aria-modal="true" aria-labelledby="entry-title" hidden>
    <p class="gate-logo">TŒRN</p>
    <h1 class="entry-title" id="entry-title">Do you want to visit the mobile version now?</h1>
    <p class="entry-note">The mobile version is still in development, and not that informative, but still fun to use.</p>
    <div class="entry-actions">
      <button type="button" id="entry-yes">Yes</button>
      <button type="button" id="entry-no">No</button>
    </div>
  </div>
  <div class="rotate-gate" id="rotate-gate" role="dialog" aria-modal="true" aria-labelledby="rotate-title" hidden>
    <p class="gate-logo">TŒRN</p>
    <div class="rotate-icon" aria-hidden="true">
      <svg viewBox="0 0 64 64">
        <rect x="22" y="6" width="20" height="40" rx="3" fill="none" stroke="currentColor" stroke-width="2"/>
        <circle cx="32" cy="40" r="1.5" fill="currentColor"/>
        <path d="M46 16c6 4 9 10 9 16" fill="none" stroke="#c62828" stroke-width="2" stroke-linecap="round"/>
        <path d="M52 12l5 6-7 1.5z" fill="#c62828"/>
      </svg>
    </div>
    <p class="rotate-title" id="rotate-title">Rotate to landscape</p>
  </div>
  <div class="welcome-modal" id="welcome-modal" role="dialog" aria-modal="true" aria-labelledby="welcome-title" hidden>
    <div class="welcome-card">
      <p class="gate-logo welcome-logo">TŒRN</p>
      <p class="welcome-kicker">What's this?</p>
      <h2 class="welcome-title" id="welcome-title">TŒRN Simulator</h2>
      <p>This is an early browser emulator of the TŒRN groovebox — meant to show the concept and how the instrument feels to play.</p>
      <p>It is a work in progress. Timing, sound, menus, and hardware details will differ from the real device. Treat it as a sketch, not a finished product.</p>
      <div class="welcome-actions">
        <button type="button" class="welcome-dismiss" id="welcome-dismiss">Got it</button>
        <label class="welcome-remember">
          <input type="checkbox" id="welcome-hide"> Don't show again
        </label>
      </div>
    </div>
  </div>
  <div class="crosshair" aria-hidden="true"><span class="crosshair-x"></span><span class="crosshair-y"></span></div>
  <aside class="key-legend" aria-label="Keyboard shortcuts">
    <div class="key-brand">
      <p class="key-brand-logo">TŒRN</p>
      <p class="key-brand-sub">Simulator 1.0</p>
      <button type="button" class="key-brand-what" id="welcome-open">What's this?</button>
    </div>
    <h1 class="jump-title">Keys</h1>
    <ul class="key-legend-list">
      <li><kbd>1</kbd>–<kbd>8</kbd> voice</li>
      <li><kbd>S</kbd> single mode</li>
      <li><kbd>F</kbd> filter</li>
      <li><kbd>L</kbd> load sample</li>
      <li><kbd>D</kbd> clear page</li>
      <li><kbd>Esc</kbd> leave</li>
      <li class="key-legend-note">Focus the grid for device keys. Tab in the menu moves between controls.</li>
      <li><kbd>Q</kbd>/<kbd>↑</kbd>, <kbd>W</kbd>/<kbd>↓</kbd> [<kbd>Tab</kbd>] encoder 1</li>
      <li><kbd>R</kbd>, <kbd>T</kbd> [<kbd>⌥</kbd>/<kbd>⇧</kbd>] encoder 2</li>
      <li><kbd>U</kbd>, <kbd>I</kbd> [<kbd>Space</kbd>] encoder 3</li>
      <li><kbd>P</kbd>/<kbd>←</kbd>, <kbd>Ü</kbd>/<kbd>→</kbd> [<kbd>Enter</kbd>] encoder 4</li>
    </ul>
    <hr>
    <div class="assist" id="assist-panel" hidden>
      <h2 class="jump-title" id="assist-heading">Assistive</h2>
      <p class="assist-live" id="assist-live" aria-live="polite" aria-atomic="true"></p>
      <p class="assist-cursor" id="assist-cursor">Cursor: —</p>
      <p class="assist-menu" id="assist-menu" hidden></p>
      <div class="assist-transport" role="group" aria-label="Transport">
        <button type="button" id="assist-play">Play / Stop</button>
        <button type="button" id="assist-paint">Paint cell</button>
        <button type="button" id="assist-erase">Erase cell</button>
        <button type="button" id="assist-clear">Clear page</button>
      </div>
      <div class="assist-voices" role="group" aria-label="Select voice">
        ${[1, 2, 3, 4, 5, 6, 7, 8].map((ch) => `<button type="button" data-assist-voice="${ch}">CH${ch}</button>`).join("")}
      </div>
      <h3 class="assist-heading">Notes on this page</h3>
      <ul class="assist-notes" id="assist-notes"></ul>
      <label class="jump-led"><span>LED simulation</span><input type="checkbox" id="led-style" checked></label>
      <label class="jump-led"><span>High contrast</span><input type="checkbox" id="a11y-contrast"></label>
    </div>
  </aside>
  <main class="device" id="device">
    <div class="chassis-wrap">
      <div class="rig">
      <aside class="quicknav">
        <div class="voices" role="group" aria-label="Voices">${voiceButtons}</div>
        <button type="button" class="quicknav-logo" aria-label="Toggle fullscreen">Œ</button>
      </aside>
      <div class="chassis">
        <p class="soon" hidden>coming soon</p>
        <div class="panel" style="height:${py(262)}" aria-hidden="true">
          <div class="joints joints--l"></div>
          <div class="joints joints--r"></div>
        </div>
        <div class="box" style="top:${py(248)}" aria-hidden="true">
          <div class="ledge"></div>
          <div class="joints joints--l"></div>
          <div class="joints joints--r"></div>
          <div class="joints joints--b"></div>
          <div class="engrave">TŒRN</div>
        </div>
        <div class="touch-shelf" aria-hidden="true"></div>
        <div class="midi-assign" id="midi-assign" hidden></div>
        <div class="matrix-bed" style="left:${px(MATRIX.x)};top:${py(MATRIX.y)};width:${px(MATRIX.w)};height:${py(MATRIX.h)}">
          <canvas id="matrix" tabindex="0" role="application" aria-label="TŒRN grid. Focus here for device keys. Arrow keys move, Enter paints, Space plays."></canvas>
        </div>
        <button type="button" class="pad" data-touch="1" style="left:${px(60)};top:${py(266)}" aria-label="Touch 1, Single mode and Exit" title="Touch 1"><span>T1</span><span class="io-label pad-hint pad-hint--r">Singlemode + Exit</span></button>
        <button type="button" class="pad" data-touch="2" style="left:${px(420)};top:${py(266)}" aria-label="Touch 2, Menu" title="Touch 2"><span>T2</span><span class="io-label pad-hint pad-hint--l">Menu</span></button>
        ${encoders}
        <button type="button" class="pad pad--front" data-touch="3" style="left:${px(420)}" aria-label="Touch 3, hold to record into voice" title="Touch 3 (hold to record)"><span>T3</span><span class="io-label">Hold to record</span></button>
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
        <nav class="mobile-shortcuts" hidden aria-label="Shortcuts">
          <button type="button" data-shortcut="single">Single</button>
          <button type="button" data-shortcut="wav">Sample</button>
          <button type="button" data-shortcut="filter">Filter</button>
          <button type="button" data-shortcut="clear">Clear</button>
          <button type="button" data-shortcut="menu">Menu</button>
        </nav>
      </div>
      </div>
    </div>
  </main>
  <aside class="jump" id="jump">
    <div id="jump-home">
    <h1 class="jump-title">Menu</h1>
    <button type="button" data-jump="empty">Empty New Track</button>
    <hr>
    <button type="button" data-jump="draw">Draw</button>
    <button type="button" data-jump="single">SingleMode</button>
    <button type="button" data-jump="menu">Open MainMenu</button>
    <button type="button" data-jump="filter">FilterMode</button>
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
    <button type="button" id="import-midi">Import MIDI File</button>
    <button type="button" id="transfer-open">Transfer SIM&lt;&gt;DEVICE</button>
    <hr>
    <p class="status" id="status" role="status"></p>
    <p class="jump-help" id="jump-help"></p>
    <div class="jump-footer">
      <label class="jump-led">
        <span id="build-version-label">BuildVersion · V2</span>
        <input type="checkbox" id="build-version" role="switch" aria-labelledby="build-version-label" checked>
      </label>
      <label class="jump-led jump-led--inline">
        <span id="assist-toggle-label">Assistive controls</span>
        <input type="checkbox" id="assist-toggle" role="switch" aria-labelledby="assist-toggle-label" aria-controls="assist-panel" aria-describedby="assist-toggle-hint">
      </label>
      <p class="jump-panel-meta muted" id="assist-toggle-hint">Screen reader announcements, transport buttons, and a text list of notes on the page. Open with <code>#assist</code> in the URL.</p>
      <a class="jump-mobile-link" id="mobile-link" href="#mobile">Mobile / iPad</a>
    </div>
    </div>
    <div class="jump-layer" id="import-layer" hidden>
      <div class="jump-layer-head">
        <button type="button" id="import-back">← Menu</button>
        <h2 class="jump-title">Import MIDI</h2>
      </div>
      <label class="jump-drop" id="import-drop">
        <input type="file" id="import-file" accept=".mid,.midi,audio/midi" hidden>
        <span id="import-drop-label">Drop .mid or browse</span>
      </label>
      <div class="jump-layer-body" id="import-settings" hidden>
        <p class="jump-panel-meta" id="import-meta"></p>
        <p class="jump-panel-meta" id="import-pages"></p>
        <div class="jump-panel-row">
          <label>BPM
            <input type="number" id="import-bpm" min="30" max="300" step="0.0001">
            <span class="jump-readout" id="import-bpm-readout">120,0000</span>
          </label>
          <label>Grid
            <select id="import-sub">
              <option value="4">1/4</option>
              <option value="8">1/8</option>
              <option value="16" selected>1/16</option>
              <option value="32">1/32</option>
            </select>
          </label>
        </div>
        <label class="jump-field">Time offset (seconds)
          <input type="range" id="import-offset" min="-8" max="8" step="0.01" value="0">
          <span class="jump-readout" id="import-offset-readout">0.00</span>
        </label>
        <label class="jump-panel-check">
          <input type="checkbox" id="import-transpose" checked> Transpose out-of-range notes
        </label>
        <label class="jump-panel-check">
          <input type="checkbox" id="import-overlap" checked> Filter overlapping notes
        </label>
        <p class="jump-panel-meta muted">Drag a track onto a voice slot, or use Assign below (CH1–8, 11, 13, 14).</p>
        <ul class="jump-tracks" id="import-tracks"></ul>
        <button type="button" class="jump-panel-reset" id="import-reset">Reset to MIDI</button>
      </div>
      <p class="jump-panel-err" id="import-err" role="alert" hidden></p>
    </div>
    <div class="jump-layer" id="sd-layer" hidden>
      <div class="jump-layer-head">
        <button type="button" id="sd-back">← Menu</button>
        <h2 class="jump-title sd-title">Transfer SIM&lt;&gt;DEVICE</h2>
      </div>
      <div class="sd-toolbar">
        <span class="jump-readout" id="sd-status" role="status">offline</span>
        <button type="button" class="jump-panel-reset" id="sd-disconnect" disabled>Disconnect</button>
      </div>
      <div class="sd-tabs" role="tablist" aria-label="Transfer">
        <button type="button" class="sd-tab" role="tab" id="sd-tab-pattern" aria-selected="true" aria-controls="sd-panel-pattern">Current pattern</button>
        <button type="button" class="sd-tab" role="tab" id="sd-tab-file" aria-selected="false" aria-controls="sd-panel-file">Pattern file</button>
      </div>
      <div id="sd-panel-pattern" role="tabpanel" aria-labelledby="sd-tab-pattern">
        <p class="jump-panel-meta muted">Replaces the current pattern in RAM. Nothing is written to a file.</p>
        <div class="sd-actions">
          <button type="button" class="jump-panel-reset" id="sd-push">SIM &gt; DEVICE</button>
          <button type="button" class="jump-panel-reset" id="sd-pull-slot">DEVICE &gt; SIM</button>
        </div>
      </div>
      <div id="sd-panel-file" role="tabpanel" aria-labelledby="sd-tab-file" hidden>
        <p class="jump-panel-meta muted">Browse and replace pattern files on the SD card.</p>
        <div class="sd-crumbs" id="sd-crumbs"></div>
        <div class="sd-list" id="sd-list" role="listbox" aria-label="SD card files"></div>
        <div class="sd-actions">
          <button type="button" class="jump-panel-reset" id="sd-pull" disabled>Replace sim with selected file</button>
        </div>
        <label class="jump-drop" id="sd-upload">
          <input type="file" id="sd-upload-file" multiple hidden>
          <span>Replace same name in this folder</span>
        </label>
      </div>
      <p class="jump-panel-err" id="sd-err" role="alert" hidden></p>
      <p class="jump-panel-meta" id="sd-xfer" role="status"></p>
    </div>
  </aside>
  <div class="xfer-modal" id="xfer-modal" hidden>
    <div class="xfer-modal-card" role="dialog" aria-modal="true" aria-labelledby="xfer-modal-text">
      <p id="xfer-modal-text"></p>
      <div class="xfer-modal-actions">
        <button type="button" id="xfer-cancel">Cancel</button>
        <button type="button" id="xfer-ok">Replace</button>
      </div>
    </div>
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
const assistToggle = document.querySelector("#assist-toggle");
const assistPanel = document.querySelector("#assist-panel");
const assistLive = document.querySelector("#assist-live");
const assistCursor = document.querySelector("#assist-cursor");
const assistMenu = document.querySelector("#assist-menu");
const assistNotes = document.querySelector("#assist-notes");
let assistLastLive = "";
let assistNotesSig = "";
let assistEnabled = false;

function setAssistEnabled(on) {
  assistEnabled = !!on;
  assistToggle.checked = assistEnabled;
  assistToggle.setAttribute("aria-checked", assistEnabled ? "true" : "false");
  assistPanel.hidden = !assistEnabled;
  document.documentElement.classList.toggle("assist-on", assistEnabled);
  device.setAssistKeyGate(assistEnabled);
  localStorage.setItem("toern-web-assist", assistEnabled ? "1" : "0");
  if (assistEnabled) {
    assistLastLive = "";
    assistNotesSig = "";
    refreshAssistive(true);
    device.announceState();
  } else {
    assistLive.textContent = "";
    assistLastLive = "";
    setDeviceFocused(true);
  }
}

device.onAnnounce((text) => {
  if (!assistEnabled) return;
  if (text === assistLastLive) return;
  assistLastLive = text;
  assistLive.textContent = text;
  refreshAssistive(false);
});

function setDeviceFocused(on) {
  device.setDeviceFocus(on);
  canvas.classList.toggle("device-focused", on);
}

canvas.addEventListener("focus", () => setDeviceFocused(true));
canvas.addEventListener("blur", () => {
  // Keep device keys if focus moved to a voice/pad on the chassis.
  const ae = document.activeElement;
  if (ae?.closest?.(".device") && ae !== canvas) setDeviceFocused(true);
  else setDeviceFocused(false);
});
document.querySelector(".device")?.addEventListener("pointerdown", () => {
  if (document.activeElement !== canvas) canvas.focus({ preventScroll: true });
  setDeviceFocused(true);
});
document.querySelector(".jump")?.addEventListener("focusin", () => setDeviceFocused(false));
document.querySelector(".key-legend")?.addEventListener("focusin", () => setDeviceFocused(false));

function refreshAssistive(forceNotes = true) {
  if (!assistEnabled) return;
  const cur = device.cursor();
  const cell = device.cellAtCursor();
  const grid = device.gridMode();
  if (grid && cur.on) {
    const cellTxt = cell.empty ? "empty" : `voice ${cell.channel}, velocity ${cell.velocity}`;
    assistCursor.textContent = `Cursor: step ${cell.step}, row ${cell.row}, page ${cell.page} — ${cellTxt}`;
    assistMenu.hidden = true;
  } else {
    const menu = device.menuSummary();
    assistCursor.textContent = `Mode: ${menu.title}`;
    assistMenu.hidden = !menu.detail;
    assistMenu.textContent = menu.detail || "";
  }
  if (!forceNotes && !grid) return;
  const notes = device.pageNotes();
  const sig = `${device.page()}:${notes.map((n) => `${n.col}-${n.row}-${n.channel}-${n.velocity}`).join(",")}`;
  if (sig === assistNotesSig && !forceNotes) return;
  assistNotesSig = sig;
  if (!notes.length) {
    assistNotes.innerHTML = `<li class="assist-empty">No notes on this page.</li>`;
    return;
  }
  assistNotes.innerHTML = notes.map((n) => `
    <li>
      <button type="button" class="assist-note" data-col="${n.col}" data-row="${n.row}" aria-label="Step ${n.col}, row ${n.row}, voice ${n.channel}, velocity ${n.velocity}">
        Step ${n.col}, row ${n.row} — CH${n.channel} vel ${n.velocity}
      </button>
    </li>`).join("");
  assistNotes.querySelectorAll(".assist-note").forEach((btn) => {
    btn.addEventListener("click", () => {
      device.moveCursor(Number(btn.dataset.col), Number(btn.dataset.row));
      canvas.focus({ preventScroll: true });
      setDeviceFocused(true);
    });
  });
}

document.querySelector("#assist-play")?.addEventListener("click", () => device.togglePlay());
document.querySelector("#assist-paint")?.addEventListener("click", () => device.paintAtCursor());
document.querySelector("#assist-erase")?.addEventListener("click", () => device.eraseAtCursor());
document.querySelector("#assist-clear")?.addEventListener("click", () => device.clearPage());
document.querySelectorAll("[data-assist-voice]").forEach((btn) => {
  btn.addEventListener("click", () => device.selectVoice(Number(btn.dataset.assistVoice)));
});

assistToggle.addEventListener("change", () => setAssistEnabled(assistToggle.checked));

function urlWantsAssist() {
  const hash = (location.hash || "").replace(/^#/, "");
  if (!hash) return false;
  return hash.split(/[&/]/).some((part) => {
    const key = part.split("=")[0].trim().toLowerCase();
    if (key !== "assist") return false;
    const val = part.includes("=") ? part.slice(part.indexOf("=") + 1).toLowerCase() : "1";
    return val !== "0" && val !== "false" && val !== "off";
  });
}

function syncAssistFromUrl() {
  if (urlWantsAssist()) setAssistEnabled(true);
}

setAssistEnabled(urlWantsAssist() || localStorage.getItem("toern-web-assist") === "1");
window.addEventListener("hashchange", syncAssistFromUrl);

let mobileUi = false;

function urlWantsMobile() {
  const hash = (location.hash || "").replace(/^#/, "");
  if (!hash) return false;
  return hash.split(/[&/]/).some((part) => {
    const key = part.split("=")[0].trim().toLowerCase();
    if (key !== "mobile") return false;
    const val = part.includes("=") ? part.slice(part.indexOf("=") + 1).toLowerCase() : "1";
    return val !== "0" && val !== "false" && val !== "off";
  });
}

async function setMobileUi(on) {
  mobileUi = !!on;
  document.documentElement.classList.toggle("mobile-ui", mobileUi);
  try {
    if (mobileUi) {
      const root = document.documentElement;
      if (!document.fullscreenElement && root.requestFullscreen) await root.requestFullscreen();
      else if (!document.fullscreenElement && root.webkitRequestFullscreen) root.webkitRequestFullscreen();
    } else if (document.fullscreenElement) {
      await document.exitFullscreen?.();
    } else if (document.webkitFullscreenElement) {
      document.webkitExitFullscreen?.();
    }
  } catch { /* iOS / blocked */ }
}

document.querySelector("#mobile-link")?.addEventListener("click", () => {
  const root = document.documentElement;
  if (!document.fullscreenElement && root.requestFullscreen) root.requestFullscreen().catch(() => {});
  else if (!document.fullscreenElement && root.webkitRequestFullscreen) root.webkitRequestFullscreen();
});
document.querySelector(".quicknav-logo")?.addEventListener("click", () => {
  const root = document.documentElement;
  const active = document.fullscreenElement || document.webkitFullscreenElement;
  if (active) {
    (document.exitFullscreen || document.webkitExitFullscreen)?.call(document);
    return;
  }
  if (root.requestFullscreen) root.requestFullscreen().catch(() => {});
  else if (root.webkitRequestFullscreen) root.webkitRequestFullscreen();
});

function syncMobileFromUrl() {
  setMobileUi(urlWantsMobile()).then(() => markVoice());
}

localStorage.removeItem("toern-web-mobile");
syncMobileFromUrl();
window.addEventListener("hashchange", syncMobileFromUrl);

const a11yContrast = document.querySelector("#a11y-contrast");
if (a11yContrast) {
  a11yContrast.checked = localStorage.getItem("toern-web-a11y-contrast") === "1";
  document.documentElement.classList.toggle("a11y-contrast", a11yContrast.checked);
  a11yContrast.addEventListener("change", () => {
    localStorage.setItem("toern-web-a11y-contrast", a11yContrast.checked ? "1" : "0");
    document.documentElement.classList.toggle("a11y-contrast", a11yContrast.checked);
  });
}

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
  if (drag === null) return;
  if (e.buttons === 0) return;
  const { col, row } = cellFromEvent(e);
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
  const pressPad = () => {
    pad.classList.add("on");
    device.touch(id, true);
  };
  const releasePad = () => {
    pad.classList.remove("on");
    device.touch(id, false);
  };
  pad.addEventListener("pointerdown", (e) => {
    if (e.button !== 0) return;
    pad.setPointerCapture(e.pointerId);
    pressPad();
  });
  pad.addEventListener("pointerup", releasePad);
  pad.addEventListener("pointercancel", releasePad);
  // Keyboard / SR activate via click
  pad.addEventListener("keydown", (e) => {
    if (e.key === " " || e.key === "Enter") {
      e.preventDefault();
      pressPad();
    }
  });
  pad.addEventListener("keyup", (e) => {
    if (e.key === " " || e.key === "Enter") {
      e.preventDefault();
      releasePad();
    }
  });
});
document.querySelectorAll(".voice").forEach((btn) => {
  btn.addEventListener("click", (e) => {
    if (btn.dataset.skipClick === "1") {
      btn.dataset.skipClick = "";
      e.preventDefault();
      return;
    }
    device.selectVoice(Number(btn.dataset.ch));
  });
});

const VOICE_LONG_MS = 420;
let voiceGesture = null;
function voiceUnder(x, y) {
  const nav = document.querySelector(".quicknav");
  const buttons = [...document.querySelectorAll(".voice")];
  if (!nav || !buttons.length) return null;
  const box = nav.getBoundingClientRect();
  if (x < box.left - 12 || x > box.right + 12 || y < box.top - 12 || y > box.bottom + 12) return null;
  let best = null;
  let bestD = Infinity;
  for (const b of buttons) {
    const r = b.getBoundingClientRect();
    const d = Math.abs(y - (r.top + r.height / 2));
    if (d < bestD) { bestD = d; best = b; }
  }
  return best;
}
const quicknav = document.querySelector(".quicknav");
function blockMobileChrome(e) {
  if (!document.documentElement.classList.contains("mobile-ui")) return;
  e.preventDefault();
}
["contextmenu", "dragstart", "selectstart", "gesturestart"].forEach((type) => {
  document.addEventListener(type, blockMobileChrome, { capture: true });
});
quicknav?.addEventListener("selectstart", (e) => e.preventDefault());
quicknav?.addEventListener("contextmenu", (e) => e.preventDefault());
document.querySelector(".voices")?.addEventListener("pointerdown", (e) => {
  const btn = e.target.closest?.(".voice");
  if (!btn || e.button !== 0) return;
  e.preventDefault();
  e.currentTarget.setPointerCapture?.(e.pointerId);
  const ch = Number(btn.dataset.ch);
  const onMute = !!e.target.closest?.(".voice-mute");
  const g = {
    id: e.pointerId,
    origin: btn,
    startCh: ch,
    ch,
    x: e.clientX,
    y: e.clientY,
    onMute,
    drag: false,
    solo: false,
    latched: false,
    timer: setTimeout(() => {
      if (!voiceGesture || voiceGesture.drag) return;
      voiceGesture.long = true;
      voiceGesture.latched = true;
      btn.dataset.skipClick = "1";
      if (voiceGesture.onMute) device.soloHold(ch);
      else device.toggleMute(ch);
    }, VOICE_LONG_MS),
  };
  voiceGesture = g;
});
document.querySelector(".voices")?.addEventListener("pointermove", (e) => {
  const g = voiceGesture;
  if (!g || e.pointerId !== g.id) return;
  if (!g.drag && Math.hypot(e.clientX - g.x, e.clientY - g.y) > 8) {
    g.drag = true;
    g.solo = true;
    clearTimeout(g.timer);
    g.origin.dataset.skipClick = "1";
    device.soloHold(g.startCh);
  }
  if (!g.drag || g.long) return;
  const hit = voiceUnder(e.clientX, e.clientY);
  if (!hit) return;
  const ch = Number(hit.dataset.ch);
  if (ch !== g.ch) {
    g.ch = ch;
    device.soloHold(ch);
  }
});
function endVoiceGesture(e) {
  const g = voiceGesture;
  if (!g || e.pointerId !== g.id) return;
  clearTimeout(g.timer);
  voiceGesture = null;
  if (g.origin) g.origin.dataset.skipClick = "1";
  if (g.latched) return;
  if (g.solo) device.soloRelease();
  else if (!g.long && !g.drag && g.onMute) device.unmute(g.startCh);
  else if (!g.long && !g.drag) device.selectVoice(g.startCh);
}
document.querySelector(".voices")?.addEventListener("pointerup", endVoiceGesture);
document.querySelector(".voices")?.addEventListener("pointercancel", endVoiceGesture);
const buildVersion = document.querySelector("#build-version");
const buildVersionLabel = document.querySelector("#build-version-label");
function syncBuildVersion(wide) {
  const isV2 = !!wide;
  buildVersion.checked = isV2;
  buildVersion.setAttribute("aria-checked", isV2 ? "true" : "false");
  buildVersionLabel.textContent = `BuildVersion · ${isV2 ? "V2" : "V1"}`;
}
buildVersion.addEventListener("change", () => {
  device.setDevice(buildVersion.checked ? 2 : 1);
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
  let gesture = null;
  // Linear drag like common web knobs: vertical (and horizontal) distance in
  // pixels, not angular travel around the ring centre.
  const DETENT_PX = 14;
  encoder.addEventListener("pointerdown", (e) => {
    if (e.button !== 0) return;
    encoder.classList.add("pressed");
    device.knobDown(i);
    gesture = {
      x: e.clientX,
      y: e.clientY,
      lastX: e.clientX,
      lastY: e.clientY,
      acc: 0,
      turned: false,
    };
    encoder.setPointerCapture(e.pointerId);
  });
  encoder.addEventListener("pointermove", (e) => {
    if (!gesture) return;
    const moved = Math.hypot(e.clientX - gesture.x, e.clientY - gesture.y);
    if (moved < 6) return;
    const dx = e.clientX - gesture.lastX;
    const dy = e.clientY - gesture.lastY;
    gesture.lastX = e.clientX;
    gesture.lastY = e.clientY;
    // Down / right → same direction as wheel-down; up / left → opposite.
    gesture.acc += dy + dx;
    const sense = i === 0 && device.gridMode() ? -1 : 1;
    while (gesture.acc >= DETENT_PX) {
      device.rotate(i, sense);
      gesture.acc -= DETENT_PX;
      gesture.turned = true;
    }
    while (gesture.acc <= -DETENT_PX) {
      device.rotate(i, -sense);
      gesture.acc += DETENT_PX;
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
    if (gesture) device.knobUp(i, !!gesture.turned);
    gesture = null;
  });
  let wheelAcc = 0;
  encoder.addEventListener("wheel", (e) => {
    e.preventDefault();
    let dy = e.deltaY;
    if (e.deltaMode === 1) dy *= 40;
    else if (e.deltaMode === 2) dy *= 800;
    wheelAcc += dy;
    const sense = i === 0 && device.gridMode() ? -1 : 1;
    const notch = 50;
    while (wheelAcc >= notch) {
      device.rotate(i, sense);
      wheelAcc -= notch;
    }
    while (wheelAcc <= -notch) {
      device.rotate(i, -sense);
      wheelAcc += notch;
    }
  }, { passive: false });
});

const encodersEl = [...document.querySelectorAll(".encoder")];
function pressKey(e) {
  if (e.repeat) return;
  if (e.code === "Tab" || e.key === "Tab") return 0;
  if (e.key === "Alt" || e.code === "AltLeft" || e.code === "AltRight"
    || e.key === "Shift" || e.code === "ShiftLeft" || e.code === "ShiftRight") return 1;
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

document.querySelectorAll(".jump button[data-jump]").forEach((btn) => {
  btn.addEventListener("click", () => device.jump(btn.dataset.jump));
});
document.querySelectorAll(".mobile-shortcuts [data-shortcut]").forEach((btn) => {
  btn.addEventListener("click", () => {
    const name = btn.dataset.shortcut;
    if (name === "clear") device.clearPage();
    else if (name === "single" && device.mode() === "single") device.jump("draw");
    else device.jump(name);
  });
});
const loadTrack = document.querySelector("#load-track");
const loadTrackSub = document.querySelector("#load-track-sub");
const jumpHome = document.querySelector("#jump-home");
const importMidiBtn = document.querySelector("#import-midi");
const importLayer = document.querySelector("#import-layer");
const importBack = document.querySelector("#import-back");
const importDrop = document.querySelector("#import-drop");
const importFile = document.querySelector("#import-file");
const importDropLabel = document.querySelector("#import-drop-label");
const importSettings = document.querySelector("#import-settings");
const importMeta = document.querySelector("#import-meta");
const importPages = document.querySelector("#import-pages");
const importBpm = document.querySelector("#import-bpm");
const importBpmReadout = document.querySelector("#import-bpm-readout");
const importSub = document.querySelector("#import-sub");
const importOffset = document.querySelector("#import-offset");
const importOffsetReadout = document.querySelector("#import-offset-readout");
const importTranspose = document.querySelector("#import-transpose");
const importOverlap = document.querySelector("#import-overlap");
const importTracks = document.querySelector("#import-tracks");
const importReset = document.querySelector("#import-reset");
const importErr = document.querySelector("#import-err");
const midiAssign = document.querySelector("#midi-assign");

let midiDraft = null;
let importTimer = 0;
let importDragTrack = null;

const MIDI_ASSIGN_CHS = [1, 2, 3, 4, 5, 6, 7, 8, 11, 13, 14];

function defaultSlots(tracks) {
  const slots = {};
  for (const ch of MIDI_ASSIGN_CHS) slots[ch] = null;
  // Auto-fill sample voices first; synth slots stay empty until assigned.
  for (let i = 0; i < 8; i++) slots[i + 1] = tracks[i]?.id ?? null;
  return slots;
}

function trackToChannelMap() {
  const map = new Map();
  if (!midiDraft) return map;
  for (const ch of MIDI_ASSIGN_CHS) {
    const id = midiDraft.slots[ch];
    if (id != null) map.set(id, ch);
  }
  return map;
}

function assignTrackToChannel(trackId, channel) {
  if (!midiDraft || !MIDI_ASSIGN_CHS.includes(channel)) return;
  let fromCh = null;
  for (const ch of MIDI_ASSIGN_CHS) {
    if (midiDraft.slots[ch] === trackId) { fromCh = ch; break; }
  }
  const displaced = midiDraft.slots[channel];
  midiDraft.slots[channel] = trackId;
  if (fromCh != null && fromCh !== channel) midiDraft.slots[fromCh] = displaced;
}

function clearChannel(channel) {
  if (!midiDraft || !MIDI_ASSIGN_CHS.includes(channel)) return;
  midiDraft.slots[channel] = null;
}

function closeImport() {
  importLayer.hidden = true;
  jumpHome.hidden = !sdLayer?.hidden;
  importMidiBtn.classList.remove("on");
  midiAssign.hidden = true;
  midiAssign.classList.remove("on");
}

function openImport() {
  closeLoadTrack();
  closeSdPanel();
  jumpHome.hidden = true;
  importLayer.hidden = false;
  importMidiBtn.classList.add("on");
  if (midiDraft) {
    midiAssign.hidden = false;
    midiAssign.classList.add("on");
    renderMidiAssign();
  }
  document.querySelector("#import-back")?.focus();
}

function closeLoadTrack() {
  loadTrackSub.hidden = true;
  loadTrack.classList.remove("on");
}

function setImportErr(msg) {
  importErr.hidden = !msg;
  importErr.textContent = msg || "";
}

function formatBpm(v) {
  return Number(v).toFixed(4).replace(".", ",");
}

function syncBpmReadout() {
  importBpmReadout.textContent = formatBpm(Number(importBpm.value) || 0);
}

function syncOffsetReadout() {
  const v = Number(importOffset.value) || 0;
  importOffsetReadout.textContent = `${v >= 0 ? "+" : ""}${v.toFixed(2)}s`;
}

function midiSlotTop(ch, matrixBox, chassisH) {
  const y = ch + 1;
  const top = matrixBox.y + (16 - y + 0.5) * (matrixBox.h / 16);
  return `${(top / chassisH) * 100}%`;
}

function renderMidiAssign() {
  if (!midiDraft) {
    midiAssign.hidden = true;
    midiAssign.classList.remove("on");
    return;
  }
  midiAssign.hidden = false;
  midiAssign.classList.add("on");
  const byId = new Map(midiDraft.tracks.map((t) => [t.id, t]));
  midiAssign.innerHTML = MIDI_ASSIGN_CHS.map((ch) => {
    const [r, g, b] = COL[ch];
    const tid = midiDraft.slots[ch];
    const track = tid != null ? byId.get(tid) : null;
    const label = track ? track.name : `CH${ch}`;
    const filled = track ? "filled" : "empty";
    return `<button type="button" class="midi-slot ${filled}" data-ch="${ch}" style="--c:rgb(${r},${g},${b})" aria-label="${track ? `${track.name} on channel ${ch}. Click to clear.` : `Channel ${ch}, drop or assign a track`}">
      <span class="midi-slot-ch" aria-hidden="true">${ch}</span>
      <span class="midi-slot-name">${label}</span>
    </button>`;
  }).join("");

  midiAssign.querySelectorAll(".midi-slot").forEach((slot) => {
    const ch = Number(slot.dataset.ch);
    slot.addEventListener("dragover", (e) => {
      e.preventDefault();
      e.dataTransfer.dropEffect = "copy";
      slot.classList.add("drag-over");
    });
    slot.addEventListener("dragleave", () => slot.classList.remove("drag-over"));
    slot.addEventListener("drop", (e) => {
      e.preventDefault();
      slot.classList.remove("drag-over");
      const id = importDragTrack ?? Number(e.dataTransfer.getData("text/plain"));
      if (!Number.isFinite(id)) return;
      assignTrackToChannel(id, ch);
      renderImportTracks();
      renderMidiAssign();
      scheduleImportApply();
    });
    slot.addEventListener("click", () => {
      if (midiDraft.slots[ch] == null) return;
      clearChannel(ch);
      renderImportTracks();
      renderMidiAssign();
      scheduleImportApply();
    });
  });
  layoutMidiAssign();
}

function viewGeom(wide) {
  const boxTop = wide ? 248 : 466;
  const shift = boxTop - 248;
  return {
    chassisH: wide ? 440 : 658,
    boxTop,
    panelH: wide ? 262 : 480,
    encY: 352 + (wide ? 0 : shift),
    padY: 266 + (wide ? 0 : shift),
    matrixBox: wide ? { x: 12, y: 8, w: 456, h: 232 } : { x: 12, y: 12, w: 456, h: 456 },
    encX: [75, 185, 295, 405],
    t1x: 60,
    t2x: 420,
    t3x: 420,
    t3y: null,
  };
}

let mobileLayout = null;

function layoutMidiAssign() {
  if (mobileLayout) {
    const { matrixH, matrixTop, totalH } = mobileLayout;
    midiAssign.querySelectorAll(".midi-slot").forEach((slot) => {
      const y = Number(slot.dataset.ch) + 1;
      const top = (matrixTop || 0) + (16 - y + 0.5) * (matrixH / 16);
      slot.style.top = `${(top / totalH) * 100}%`;
    });
    return;
  }
  const wide = device.cols() > 16;
  const { chassisH, matrixBox } = viewGeom(wide);
  midiAssign.querySelectorAll(".midi-slot").forEach((slot) => {
    slot.style.top = midiSlotTop(Number(slot.dataset.ch), matrixBox, chassisH);
  });
}

function renderImportTracks() {
  if (!midiDraft) return;
  const assigned = trackToChannelMap();
  importTracks.innerHTML = midiDraft.tracks.map((t) => {
    const voice = assigned.get(t.id) || 0;
    const badge = voice
      ? `<span class="voice-badge">CH${voice}</span>`
      : `<span class="voice-badge skip">—</span>`;
    return `<li data-id="${t.id}" draggable="true" class="${voice ? "" : "dim"}">
      <span class="grip" aria-hidden="true"></span>
      <span class="swatch" style="background:${t.color}" aria-hidden="true"></span>
      <span class="name" id="track-name-${t.id}">${t.name}</span>
      ${badge}
      <label class="track-assign">
        <span class="sr-only">Assign ${t.name}</span>
        <select data-assign="${t.id}" aria-labelledby="track-name-${t.id}">
          <option value="">Unassigned</option>
          ${MIDI_ASSIGN_CHS.map((ch) => `<option value="${ch}" ${voice === ch ? "selected" : ""}>CH${ch}</option>`).join("")}
        </select>
      </label>
      <span class="count">${t.noteCount}</span>
    </li>`;
  }).join("");

  importTracks.querySelectorAll("li").forEach((li) => {
    const id = Number(li.dataset.id);
    li.addEventListener("dragstart", (e) => {
      importDragTrack = id;
      li.classList.add("dragging");
      e.dataTransfer.effectAllowed = "copy";
      e.dataTransfer.setData("text/plain", String(id));
    });
    li.addEventListener("dragend", () => {
      importDragTrack = null;
      li.classList.remove("dragging");
      midiAssign.querySelectorAll(".midi-slot").forEach((el) => el.classList.remove("drag-over"));
    });
    const sel = li.querySelector("select[data-assign]");
    sel?.addEventListener("change", () => {
      const ch = Number(sel.value);
      if (!ch) {
        const prev = channelOfTrack(id);
        if (prev) clearChannel(prev);
      } else {
        assignTrackToChannel(id, ch);
      }
      renderImportTracks();
      renderMidiAssign();
      scheduleImportApply();
    });
  });
}

function channelOfTrack(trackId) {
  if (!midiDraft) return 0;
  for (const ch of MIDI_ASSIGN_CHS) {
    if (midiDraft.slots[ch] === trackId) return ch;
  }
  return 0;
}

function currentImportParams() {
  const offset = Number(importOffset.value);
  return {
    bpm: Number(importBpm.value) || midiDraft?.bpm || 120,
    sub: Number(importSub.value) || 16,
    offset: Number.isFinite(offset) ? offset : 0,
    transpose: importTranspose.checked,
    overlap: importOverlap.checked,
  };
}

function applyImportNow() {
  if (!midiDraft) return;
  const { bpm, sub, offset, transpose, overlap } = currentImportParams();
  syncBpmReadout();
  let grid = mapNotesToGrid(midiDraft.notes, bpm, sub, offset, transpose);
  if (overlap) grid = filterOverlappingNotes(grid);
  const toCh = trackToChannelMap();
  const enabled = [...toCh.keys()];
  const rawCount = grid.filter((n) => toCh.has(n.track)).length;
  const { cells, pages } = buildImportCells(grid, toCh, transpose);
  const voices = toCh.size;
  importMeta.textContent = `${midiDraft.name} · ${cells.length}/${rawCount} notes · ${voices} voices`;
  if (!pages.length) importPages.textContent = "Pages taken: none";
  else if (pages.length === 1) importPages.textContent = `Pages taken: ${pages[0]}`;
  else importPages.textContent = `Pages taken: ${pages[0]}–${pages[pages.length - 1]} (${pages.length} pages)`;
  midiDraft.grid = grid;
  if (!enabled.length) {
    setImportErr("Drop at least one track onto a voice slot.");
    return;
  }
  setImportErr("");
  device.importMidi(cells, bpm);
}

function scheduleImportApply() {
  clearTimeout(importTimer);
  importTimer = setTimeout(applyImportNow, 60);
}

function resetImportToMidi() {
  if (!midiDraft) return;
  importBpm.value = midiDraft.bpm;
  importSub.value = "16";
  importOffset.value = "0";
  importTranspose.checked = true;
  importOverlap.checked = true;
  midiDraft.tracks = midiDraft.originalTracks.map((t) => ({ ...t }));
  midiDraft.slots = defaultSlots(midiDraft.tracks);
  renderImportTracks();
  renderMidiAssign();
  syncBpmReadout();
  syncOffsetReadout();
  applyImportNow();
}

async function loadMidiFile(file) {
  if (!file) return;
  setImportErr("");
  try {
    const { notes, tracks, bpm } = await parseMidiFile(file);
    if (!tracks.length) throw new Error("No note tracks in this file.");
    midiDraft = {
      name: file.name,
      notes,
      tracks: tracks.map((t) => ({ ...t })),
      originalTracks: tracks.map((t) => ({ ...t })),
      bpm,
      slots: defaultSlots(tracks),
      grid: [],
    };
    importDropLabel.textContent = file.name;
    importBpm.value = bpm;
    importSub.value = "16";
    importOffset.value = "0";
    importTranspose.checked = true;
    importOverlap.checked = true;
    importSettings.hidden = false;
    syncBpmReadout();
    syncOffsetReadout();
    renderImportTracks();
    renderMidiAssign();
    applyImportNow();
  } catch (err) {
    midiDraft = null;
    importSettings.hidden = true;
    midiAssign.hidden = true;
    midiAssign.classList.remove("on");
    setImportErr(err?.message || "Could not read that MIDI file.");
  }
}

loadTrack.addEventListener("click", () => {
  closeImport();
  loadTrackSub.hidden = !loadTrackSub.hidden;
  loadTrack.classList.toggle("on", !loadTrackSub.hidden);
});
loadTrackSub.querySelectorAll("button").forEach((btn) => {
  btn.addEventListener("click", () => device.loadGenre(Number(btn.dataset.track)));
});

importMidiBtn.addEventListener("click", () => openImport());
importBack.addEventListener("click", () => closeImport());
importDrop.addEventListener("click", () => importFile.click());
importFile.addEventListener("change", () => {
  const file = importFile.files?.[0];
  if (file) loadMidiFile(file);
  importFile.value = "";
});
["dragenter", "dragover"].forEach((type) => {
  importDrop.addEventListener(type, (e) => {
    e.preventDefault();
    importDrop.classList.add("on");
  });
});
["dragleave", "drop"].forEach((type) => {
  importDrop.addEventListener(type, (e) => {
    e.preventDefault();
    importDrop.classList.remove("on");
  });
});
importDrop.addEventListener("drop", (e) => {
  const file = [...(e.dataTransfer?.files || [])].find((f) => /\.midi?$/i.test(f.name));
  if (file) loadMidiFile(file);
  else setImportErr("Drop a .mid file.");
});
importBpm.addEventListener("input", () => { syncBpmReadout(); scheduleImportApply(); });
importSub.addEventListener("change", scheduleImportApply);
importOffset.addEventListener("input", () => { syncOffsetReadout(); scheduleImportApply(); });
importTranspose.addEventListener("change", scheduleImportApply);
importOverlap.addEventListener("change", scheduleImportApply);
importReset.addEventListener("click", resetImportToMidi);

// --- Transfer SIM<>DEVICE (sidebar: pattern RAM + SD files) ------------
const TEENSY_VID = 0x16c0;
const TEENSY_MIDI_PID = 0x0489;
const transferOpen = document.querySelector("#transfer-open");
const sdLayer = document.querySelector("#sd-layer");
const sdBack = document.querySelector("#sd-back");
const sdDisconnect = document.querySelector("#sd-disconnect");
const sdStatus = document.querySelector("#sd-status");
const sdPush = document.querySelector("#sd-push");
const sdPullSlot = document.querySelector("#sd-pull-slot");
const sdPull = document.querySelector("#sd-pull");
const sdCrumbs = document.querySelector("#sd-crumbs");
const sdList = document.querySelector("#sd-list");
const sdUpload = document.querySelector("#sd-upload");
const sdUploadFile = document.querySelector("#sd-upload-file");
const sdErr = document.querySelector("#sd-err");
const sdXfer = document.querySelector("#sd-xfer");
const sdTabPattern = document.querySelector("#sd-tab-pattern");
const sdTabFile = document.querySelector("#sd-tab-file");
const sdPanelPattern = document.querySelector("#sd-panel-pattern");
const sdPanelFile = document.querySelector("#sd-panel-file");
const xferModal = document.querySelector("#xfer-modal");
const xferText = document.querySelector("#xfer-modal-text");
const xferOk = document.querySelector("#xfer-ok");
const xferCancel = document.querySelector("#xfer-cancel");

let sd = null;
let sdPath = "/";
let sdEntries = [];
let sdSelected = null;
let sdBusy = false;
let sdSmpCache = null;
let sdAutoConnecting = false;

function setSdErr(msg) {
  sdErr.hidden = !msg;
  sdErr.textContent = msg || "";
}

function setSdStatus(text) {
  sdStatus.textContent = text;
}

function refreshSdStatus() {
  if (sd?.connected) setSdStatus(sdPath && sdPath !== "/" ? `online · ${sdPath}` : "online");
  else if (sdAutoConnecting) setSdStatus("connecting…");
  else setSdStatus("offline");
}

function setSdBusy(on) {
  sdBusy = on;
  const live = !!(sd && sd.connected);
  sdPush.disabled = on;
  sdPullSlot.disabled = on;
  sdPull.disabled = on || !sdSelected || sdSelected.type !== "file";
  sdDisconnect.disabled = on || !live;
  sdUpload.classList.toggle("dim", on);
  refreshSdStatus();
}

function showSdTab(which) {
  const pattern = which === "pattern";
  sdTabPattern.setAttribute("aria-selected", pattern ? "true" : "false");
  sdTabFile.setAttribute("aria-selected", pattern ? "false" : "true");
  sdTabPattern.classList.toggle("on", pattern);
  sdTabFile.classList.toggle("on", !pattern);
  sdPanelPattern.hidden = !pattern;
  sdPanelFile.hidden = pattern;
}

function askModal(message, okLabel) {
  return new Promise((resolve) => {
    xferText.textContent = message;
    xferOk.hidden = !okLabel;
    xferOk.textContent = okLabel || "Replace";
    xferCancel.textContent = okLabel ? "Cancel" : "Close";
    xferModal.hidden = false;
    (okLabel ? xferOk : xferCancel).focus();
    const finish = (yes) => {
      xferModal.hidden = true;
      xferOk.removeEventListener("click", onOk);
      xferCancel.removeEventListener("click", onCancel);
      document.removeEventListener("keydown", onKey);
      resolve(yes);
    };
    const onOk = () => finish(true);
    const onCancel = () => finish(false);
    const onKey = (ev) => {
      if (ev.key === "Escape") finish(false);
    };
    xferOk.addEventListener("click", onOk);
    xferCancel.addEventListener("click", onCancel);
    document.addEventListener("keydown", onKey);
  });
}

function showXferError(err) {
  return askModal(err?.message || String(err), "");
}

async function pickTeensyPort() {
  const ports = await navigator.serial.getPorts();
  const infoOf = (port) => port.getInfo?.() || {};
  const midi = ports.find((port) => {
    const info = infoOf(port);
    return info.usbVendorId === TEENSY_VID && info.usbProductId === TEENSY_MIDI_PID;
  });
  if (midi) return midi;
  const teensy = ports.find((port) => infoOf(port).usbVendorId === TEENSY_VID);
  if (teensy) return teensy;
  if (ports.length === 1) return ports[0];
  return null;
}

async function ensureTeensy({ requestIfMissing = true } = {}) {
  if (!navigator.serial) throw new Error("Web Serial needs Chrome or Edge.");

  // Port open ≠ device answering. Probe first; reconnect on a dead session.
  if (sd?.connected) {
    try {
      await sd.ping();
      setSdStatus("online");
      return true;
    } catch (err) {
      sdLogReconnect(err);
      try { await sd.close(); } catch (_) {}
      sd = null;
      refreshSdStatus();
    }
  }

  let port = await pickTeensyPort();
  if (!port) {
    if (!requestIfMissing) return false;
    port = await navigator.serial.requestPort({ filters: [{ usbVendorId: TEENSY_VID }] });
  }
  if (sd) {
    try { await sd.close(); } catch (_) {}
    sd = null;
  }
  sd = new ToernSdSerial();
  sd.onDisconnect = () => {
    sd = null;
    refreshSdStatus();
    setSdBusy(false);
  };
  setSdStatus("connecting…");
  await sd.connect(port, (msg) => setSdStatus(msg || "connecting…"));
  // Prove the host↔device line is live before claiming online.
  await sd.ping();
  setSdStatus("online");
  return true;
}

function sdLogReconnect(err) {
  try {
    console.warn("[toern-sd] stale session, reconnecting:", err?.message || err);
  } catch (_) {}
}

async function dropDeadSession(err) {
  if (!/timeout|not connected|network|FAILED|ERR /i.test(String(err?.message || err))) return;
  try { if (sd) await sd.close(); } catch (_) {}
  sd = null;
  refreshSdStatus();
}

async function disconnectSd() {
  // Always allow disconnect, even mid-transfer.
  sdBusy = false;
  setSdErr("");
  sdXfer.textContent = "";
  setSdStatus("disconnecting…");
  const session = sd;
  sd = null;
  try {
    if (session) await session.close();
  } catch (_) {}
  sdEntries = [];
  sdSelected = null;
  renderSdList();
  setSdStatus("offline");
  setSdBusy(false);
}

function openSdPanel() {
  closeImport();
  closeLoadTrack();
  jumpHome.hidden = true;
  sdLayer.hidden = false;
  transferOpen.classList.add("on");
  showSdTab("pattern");
  setSdErr("");
  sdXfer.textContent = "";
  refreshSdStatus();
  sdBack.focus();
}

function closeSdPanel() {
  sdLayer.hidden = true;
  transferOpen.classList.remove("on");
  if (importLayer.hidden) jumpHome.hidden = false;
}

function showDraw() {
  device.jump("draw");
  canvas.focus({ preventScroll: true });
}

function renderSdCrumbs() {
  const parts = sdPath === "/" ? [] : sdPath.replace(/^\/+|\/+$/g, "").split("/");
  let html = `<button type="button" data-path="/">/</button>`;
  let acc = "";
  for (const part of parts) {
    acc += "/" + part;
    html += `<span class="sep">/</span><button type="button" data-path="${acc}">${part}</button>`;
  }
  sdCrumbs.innerHTML = html;
  sdCrumbs.querySelectorAll("button").forEach((btn) => {
    btn.addEventListener("click", () => sdBrowse(btn.dataset.path));
  });
}

function fmtSize(n) {
  if (n == null) return "";
  if (n < 1024) return `${n} B`;
  if (n < 1024 * 1024) return `${(n / 1024).toFixed(1)} K`;
  return `${(n / (1024 * 1024)).toFixed(2)} M`;
}

function renderSdList() {
  renderSdCrumbs();
  if (!sdEntries.length) {
    sdList.innerHTML = `<div class="sd-row dim" role="option" aria-disabled="true">Empty folder</div>`;
    return;
  }
  sdList.innerHTML = sdEntries.map((e, i) => {
    const sel = sdSelected && sdSelected.name === e.name && sdSelected.type === e.type;
    const icon = e.type === "dir" ? "DIR" : "FILE";
    const label = e.type === "dir"
      ? `Folder ${e.name}`
      : `File ${e.name}${e.size != null ? `, ${fmtSize(e.size)}` : ""}`;
    return `<button type="button" class="sd-row ${sel ? "selected" : ""}" role="option" aria-selected="${sel ? "true" : "false"}" data-i="${i}" aria-label="${label}">
      <span class="sd-kind" aria-hidden="true">${icon}</span>
      <span class="sd-name">${e.name}</span>
      <span class="sd-size" aria-hidden="true">${e.type === "file" ? fmtSize(e.size) : ""}</span>
    </button>`;
  }).join("");
  sdList.querySelectorAll(".sd-row[data-i]").forEach((btn) => {
    btn.addEventListener("click", () => {
      const e = sdEntries[Number(btn.dataset.i)];
      if (!e) return;
      if (e.type === "dir") {
        sdBrowse(joinSdPath(sdPath, e.name));
        return;
      }
      sdSelected = e;
      renderSdList();
      setSdBusy(sdBusy);
    });
    btn.addEventListener("dblclick", async () => {
      const e = sdEntries[Number(btn.dataset.i)];
      if (!e || e.type !== "file") return;
      sdSelected = e;
      await loadSdFileToSim(joinSdPath(sdPath, e.name));
    });
    btn.addEventListener("keydown", (ev) => {
      if (ev.key !== "Enter") return;
      const e = sdEntries[Number(btn.dataset.i)];
      if (!e || e.type !== "file") return;
      ev.preventDefault();
      loadSdFileToSim(joinSdPath(sdPath, e.name));
    });
  });
}

async function sdBrowse(path) {
  setSdBusy(true);
  setSdErr("");
  try {
    await ensureTeensy();
    sdPath = path || "/";
    sdSelected = null;
    sdEntries = await sd.listDir(sdPath);
    renderSdList();
    setSdStatus(`online · ${sdPath}`);
  } catch (err) {
    if (err?.name !== "NotFoundError") {
      await dropDeadSession(err);
      setSdErr(err?.message || String(err));
    }
  } finally {
    setSdBusy(false);
  }
}

async function pushPatternRam() {
  if (sdBusy) return;
  const ok = await askModal("Replace the current pattern in device RAM with the sim pattern? This does not write a file.", "Replace");
  if (!ok) return;
  setSdBusy(true);
  setSdErr("");
  sdXfer.textContent = "";
  try {
    await ensureTeensy();
    const bytes = encodePatternRam(device.exportPatternCells(), device.bpm());
    await sd.putPatternRam(bytes, (done, total) => {
      sdXfer.textContent = `SIM → DEVICE… ${Math.round((done / total) * 100)}%`;
    });
    sdXfer.textContent = "Device RAM updated.";
    setSdStatus("online");
    showDraw();
  } catch (err) {
    if (err?.name === "NotFoundError") return;
    await dropDeadSession(err);
    await showXferError(err);
    setSdErr(err?.message || String(err));
    sdXfer.textContent = "";
  } finally {
    setSdBusy(false);
  }
}

async function pullPatternRam() {
  if (sdBusy) return;
  const ok = await askModal("Replace the current pattern in sim RAM with the device pattern?", "Replace");
  if (!ok) return;
  setSdBusy(true);
  setSdErr("");
  sdXfer.textContent = "";
  try {
    await ensureTeensy();
    const bytes = await sd.getPatternRam((done, total) => {
      sdXfer.textContent = `DEVICE → SIM… ${Math.round((done / total) * 100)}%`;
    });
    const { cells, bpm } = decodePatternFile(bytes);
    device.importPatternCells(cells, bpm);
    sdXfer.textContent = `Sim RAM updated${bpm ? ` @ ${bpm.toFixed(1)} BPM` : ""}.`;
    setSdStatus("online");
    showDraw();
  } catch (err) {
    if (err?.name === "NotFoundError") return;
    await dropDeadSession(err);
    await showXferError(err);
    setSdErr(err?.message || String(err));
    sdXfer.textContent = "";
  } finally {
    setSdBusy(false);
  }
}

async function loadSdFileToSim(remote) {
  setSdBusy(true);
  setSdErr("");
  sdXfer.textContent = "";
  try {
    await ensureTeensy();
    const bytes = await sd.getBytes(remote, (done, total) => {
      sdXfer.textContent = `Reading ${remote}… ${Math.round((done / total) * 100)}%`;
    });
    const { cells, bpm, smpTail } = decodePatternFile(bytes);
    if (smpTail?.length) sdSmpCache = smpTail;
    device.importPatternCells(cells, bpm);
    sdXfer.textContent = `Loaded ${remote} into sim${bpm ? ` @ ${bpm.toFixed(1)} BPM` : ""}.`;
    showDraw();
  } catch (err) {
    if (err?.name !== "NotFoundError") {
      await dropDeadSession(err);
      setSdErr(err?.message || String(err));
    }
    sdXfer.textContent = "";
  } finally {
    setSdBusy(false);
  }
}

async function uploadSdFiles(files) {
  if (!files?.length) return;
  setSdBusy(true);
  setSdErr("");
  try {
    await ensureTeensy();
    for (const file of files) {
      const remote = joinSdPath(sdPath, file.name);
      const buf = new Uint8Array(await file.arrayBuffer());
      await sd.putBytes(remote, buf, (done, total) => {
        sdXfer.textContent = `Uploading ${file.name}… ${Math.round((done / total) * 100)}%`;
      });
    }
    sdXfer.textContent = `Uploaded ${files.length} file(s).`;
    await sdBrowse(sdPath);
  } catch (err) {
    if (err?.name !== "NotFoundError") {
      await dropDeadSession(err);
      setSdErr(err?.message || String(err));
    }
  } finally {
    setSdBusy(false);
  }
}

transferOpen.addEventListener("click", () => openSdPanel());
sdBack.addEventListener("click", () => closeSdPanel());
sdDisconnect.addEventListener("click", () => disconnectSd());
sdPush.addEventListener("click", () => pushPatternRam());
sdPullSlot.addEventListener("click", () => pullPatternRam());
sdPull.addEventListener("click", () => {
  if (!sdSelected || sdSelected.type !== "file") return;
  loadSdFileToSim(joinSdPath(sdPath, sdSelected.name));
});
sdUpload.addEventListener("click", () => {
  if (sdBusy) return;
  sdUploadFile.click();
});
sdUploadFile.addEventListener("change", () => {
  const files = [...(sdUploadFile.files || [])];
  sdUploadFile.value = "";
  if (files.length) uploadSdFiles(files);
});
sdTabPattern.addEventListener("click", () => showSdTab("pattern"));
sdTabFile.addEventListener("click", async () => {
  showSdTab("file");
  await sdBrowse(sdPath || "/");
});
renderSdList();
showSdTab("pattern");
setSdBusy(false);

const HELP = {
  draw: "Draw is the grid. Move with the encoders, Enter paints a note, Space plays. Hold T3 to record into the current voice when REC→TRIG is SENS. D clears this page. T1 enters single mode on the current row. T2 opens the main menu.",
  empty: "Empty New Track clears all notes and effects (ETC → RSET → FULL), then returns to Draw.",
  single: "Single mode is one voice, and each row is a pitch. Enter paints that voice. On the bottom row, hold Tab to generate a pattern. D clears only this voice. T1 returns to draw.",
  filter: "Filter shapes the current voice. The four encoders are the controls on this page. T2 moves to the next filter page. T1 leaves. T3 resets this page.",
  menu: "Main menu. Encoder 4 moves through the pages, Enter opens the one you are on. T2 or Esc leaves.",
  wav: "Load a sample onto this voice. Encoder 4 moves through folders and files, Enter opens a folder or loads the file. Q/W and R/T trim the sample, Tab previews it. T3 records from the mic.",
  rec: "Mic record. Encoder 2 starts the take, encoder 3 stops it or plays it back, encoder 4 leaves. The recording replaces this voice.",
  recs: "REC settings. INPT picks LINE or MIC. MIC and L-IN set input gain. TRIG=SENS enables hold-T3 direct record on the grid. CLR picks OFF / ON / FIX / ON1 / CLIC note policy.",
  bpm: "Tempo. Encoder 4 sets the BPM. Tap T3 to set the tempo by hand.",
  velocity: "Velocity of the note under the cursor: encoder 1 is level, 2 is probability, 3 is condition, 4 is voice volume. Enter leaves.",
  dat: "Save or load a pattern. Encoder 4 picks the slot. Encoder 1 loads it, encoder 2 saves it. An empty slot opens NEW.",
  new: "NEW writes a pattern. Encoder 1 loads it. Encoder 2 sets how many pages (not on BLNK). Encoder 3 picks BLNK, TECH, HIPH, DNB, HOUS, or AMBT. Encoder 4 goes back to FILE.",
  kit: "Load a sound pack onto the eight voices. Encoder 4 picks the pack, encoder 2 loads it.",
  song: "Song arranges patterns in order. Encoder 4 moves through the song, encoder 2 picks the pattern. Encoder 3 starts the song.",
  look: "Settings. Encoder 4 picks a row, encoder 3 changes it. LEDS 1 and 1B use a 16×16 matrix, 2 and 2B use 32×16. Esc leaves.",
  vol: "Output levels. Encoder 4 picks main, gain, or preview, encoder 3 changes it.",
  recs: "Recording input. Encoder 4 picks a row, encoder 3 changes it.",
  midi: "MIDI. Encoder 4 picks a row, encoder 3 changes it.",
  etc: "ETC. Encoder 4 picks INFO / RAM / SD / …. Transfer SIM<>DEVICE opens the side panel for RAM and SD file copy. Esc leaves.",
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
    const on = n === ch;
    btn.classList.toggle("on", on);
    btn.classList.toggle("muted", device.muted(n));
    btn.setAttribute("aria-pressed", on ? "true" : "false");
  });
  const mode = device.mode();
  const singleBtn = document.querySelector('.mobile-shortcuts [data-shortcut="single"]');
  if (singleBtn) singleBtn.textContent = mode === "single" ? "Back" : "Single";
  const clearBtn = document.querySelector('.mobile-shortcuts [data-shortcut="clear"]');
  if (clearBtn) clearBtn.disabled = mode !== "draw" && mode !== "single";
  document.querySelector(".soon").hidden = !device.soon();
  document.querySelector(".voices").classList.toggle("on", mode === "draw" || mode === "single" || mode === "filter");
  refreshAssistive(false);
  document.querySelectorAll(".jump button[data-jump]").forEach((btn) => {
    const on = btn.dataset.jump === mode;
    btn.classList.toggle("on", on);
    btn.setAttribute("aria-pressed", on ? "true" : "false");
  });
  if (mode !== helpMode) {
    helpMode = mode;
    helpEl.textContent = HELP[mode] || "Esc leaves this screen.";
    device.announceState();
  }
  const cols = device.cols();
  const wide = cols > 16;
  document.querySelector("#device")?.classList.toggle("device--v1", !wide);
  document.querySelector("#device")?.classList.toggle("device--v2", wide);
  const ledGen = wide ? 2 : 1;
  if (ledGen !== ledLookGen) {
    ledLookGen = ledGen;
    applyLedPreset(ledGen);
  }
  const compact = document.documentElement.classList.contains("mobile-ui");
  const chassis = document.querySelector(".chassis");
  const bed = document.querySelector(".matrix-bed");
  const panel = document.querySelector(".panel");
  const box = document.querySelector(".box");
  const t1 = document.querySelector('.pad[data-touch="1"]');
  const t2 = document.querySelector('.pad[data-touch="2"]');
  const t3 = document.querySelector('.pad[data-touch="3"]');
  if (compact) {
    const wrapEl = chassis.closest(".chassis-wrap");
    const fit = (width) => {
      const UW = 480;
      const padR = UW * 0.017;
      const encR = UW * 0.05;
      const edge = UW * 0.062;
      const vGap = UW * 0.02;
      const topPad = 10;
      const botPad = 12;
      const tYu0 = topPad + padR;
      const encYu0 = tYu0 + padR + vGap + encR;
      const t3Yu0 = encYu0 + encR + vGap + padR;
      const deckU0 = t3Yu0 + padR + botPad;
      const deckScale = 0.85 * 0.7 * 1.1;
      const tYu = tYu0 * deckScale;
      const encYu = encYu0 * deckScale;
      const t3Yu = t3Yu0 * deckScale;
      const deckU = deckU0 * deckScale;
      const bump = deckU * 0.05;
      const encExtra = deckU * 0.05;
      const encDrop = deckU * (0.02 + 0.10);
      const matrixU = wide ? UW * 0.5 : UW;
      return {
        UW, padR, encR, edge,
        tYu,
        encYu: encYu + bump + encDrop,
        t3Yu: t3Yu + bump,
        deckU: deckU + bump * 2 + encExtra,
        matrixU, bump,
      };
    };
    wrapEl.style.paddingLeft = "";
    const boxW = wrapEl.clientWidth;
    const boxH = wrapEl.clientHeight;
    const guessPlan = fit(Math.max(boxW, 1));
    const guess = boxH > 0
      ? Math.min(boxW / guessPlan.UW, boxH / (guessPlan.matrixU + guessPlan.deckU))
      : boxW / guessPlan.UW;
    const muteGap = 4;
    const edgePad = 6;
    const sideCap = Math.max(56, boxW * 0.3);
    const layoutFor = (diameter) => {
      let nextD = diameter;
      const groupOf = (diameter) => 2 * diameter + muteGap;
      let sideW = Math.ceil(groupOf(nextD) + 2 * edgePad);
      if (sideW > sideCap) {
        nextD = Math.max(8, (sideCap - muteGap - 2 * edgePad) / 2);
        sideW = Math.floor(sideCap);
      }
      const maxW = Math.max(80, boxW - sideW);
      const plan = fit(maxW);
      const scale = boxH > 0
        ? Math.min(maxW / plan.UW, boxH / (plan.matrixU + plan.deckU))
        : maxW / plan.UW;
      const chassisW = plan.UW * scale;
      const matrixH = plan.matrixU * scale;
      const deckH = plan.deckU * scale;
      const totalH = Math.min(boxH || matrixH + deckH, matrixH + deckH + Math.max(0, (boxH || 0) - matrixH - deckH));
      const deckTop = totalH - deckH;
      const encVis = chassisW * 0.05;
      const encYpx = Math.min(
        totalH - encVis - 2,
        Math.max(deckTop + encVis, deckTop + plan.encYu * scale),
      );
      const matrixPad = chassisW * 0.01;
      const availW = Math.max(1, chassisW - matrixPad * 2);
      const availH = Math.max(1, deckTop);
      const aspect = chassisW / matrixH;
      let bedW = availW;
      let bedH = bedW / aspect;
      if (bedH > availH) {
        bedH = availH;
        bedW = bedH * aspect;
      }
      const bedLeft = (chassisW - bedW) / 2;
      const bedTop = Math.max(0, (deckTop - bedH) / 2);
      const row = bedH / 16;
      return { d: nextD, sideW, plan, scale, chassisW, matrixH, deckH, totalH, deckTop, encYpx, bedW, bedH, bedLeft, bedTop, row };
    };
    let laid = layoutFor(Math.max(12, Math.min(22, (guessPlan.matrixU * guess) / 16 * 0.72)));
    for (let pass = 0; pass < 2; pass++) {
      const scaledD = Math.max(8, (laid.row || 12) * 0.85);
      if (Math.abs(scaledD - laid.d) <= 0.75) break;
      laid = layoutFor(scaledD);
    }
    const { d, sideW, plan, scale, chassisW, deckH, totalH, deckTop, encYpx, bedW, bedH, bedLeft, bedTop } = laid;
    const voiceRight = Math.max(0, (sideW - (2 * d + muteGap)) / 2);
    document.documentElement.style.setProperty("--voice-d", `${d}px`);
    document.documentElement.style.setProperty("--voice-row", `${laid.row}px`);
    document.documentElement.style.setProperty("--mute-gap", `${muteGap}px`);
    document.documentElement.style.setProperty("--voice-gap", `${voiceRight}px`);
    document.documentElement.style.setProperty("--quicknav-w", `${sideW}px`);
    chassis.style.aspectRatio = "auto";
    chassis.style.width = `${chassisW}px`;
    chassis.style.height = `${totalH}px`;
    panel.style.height = `${deckTop}px`;
    box.style.top = `${deckTop}px`;
    bed.style.left = `${bedLeft}px`;
    bed.style.top = `${bedTop}px`;
    bed.style.width = `${bedW}px`;
    bed.style.height = `${bedH}px`;
    const padVis = chassisW * 0.012;
    const encVis = chassisW * 0.05;
    const t1x = Math.max(padVis + 4, (plan.edge + plan.padR) * scale);
    const t2x = Math.min(chassisW - padVis - 4, chassisW - (plan.edge + plan.padR) * scale);
    const tY = deckTop + plan.tYu * scale;
    const t3Y = Math.min(totalH - padVis - 2, deckTop + plan.t3Yu * scale);
    t1.style.left = `${t1x}px`;
    t2.style.left = `${t2x}px`;
    t3.style.left = `${(t1x + t2x) / 2}px`;
    t1.style.top = `${tY}px`;
    t2.style.top = `${tY}px`;
    t3.style.top = `${t3Y}px`;
    const shelf = document.querySelector(".touch-shelf");
    const split = Math.min(tY + padVis + 4 + plan.bump * scale, encYpx - encVis - 2);
    shelf.style.top = `${deckTop}px`;
    shelf.style.height = `${Math.max(0, split - deckTop)}px`;
    const encLeft = Math.max(encVis + 2, t1x + padVis + encVis + chassisW * 0.02);
    const encRight = Math.min(chassisW - encVis - 2, t2x - padVis - encVis - chassisW * 0.02);
    document.querySelectorAll(".encoder").forEach((el, i) => {
      const span = Math.max(encRight - encLeft, 0);
      el.style.left = `${encLeft + (span * i) / 3}px`;
      el.style.top = `${encYpx}px`;
    });
    document.querySelectorAll(".voice").forEach((btn) => {
      const row = Number(btn.dataset.ch) + 1;
      btn.style.top = `${bedTop + (16 - row + 0.5) * (bedH / 16)}px`;
    });
    const logo = document.querySelector(".quicknav-logo");
    logo.style.top = `${encYpx}px`;
    const shortcuts = document.querySelector(".mobile-shortcuts");
    const bandInset = Math.max(padVis + 28, chassisW * 0.1);
    const bandLeft = t1x + bandInset;
    const bandWidth = Math.max(0, t2x - bandInset - bandLeft);
    const shelfH = Math.max(0, split - deckTop);
    const bandH = Math.min(28, Math.max(20, shelfH - 6));
    const bandTop = Math.max(deckTop + 2, Math.min(tY - bandH / 2, split - bandH - 2));
    shortcuts.hidden = false;
    shortcuts.style.left = `${bandLeft}px`;
    shortcuts.style.width = `${bandWidth}px`;
    shortcuts.style.top = `${bandTop}px`;
    shortcuts.style.height = `${bandH}px`;
    mobileLayout = { matrixH: bedH, matrixTop: bedTop, totalH };
  } else {
    mobileLayout = null;
    const shortcuts = document.querySelector(".mobile-shortcuts");
    shortcuts.hidden = true;
    shortcuts.style.left = "";
    shortcuts.style.width = "";
    shortcuts.style.top = "";
    shortcuts.style.height = "";
    chassis.parentElement.style.paddingLeft = "";
    const g = viewGeom(wide);
    const { chassisH, boxTop, panelH, encY, padY, matrixBox, encX, t1x, t2x, t3x } = g;
    const shift = boxTop - 248;
    const pyNow = (y) => `${(y / chassisH) * 100}%`;
    chassis.style.aspectRatio = `480 / ${chassisH}`;
    chassis.style.width = "";
    chassis.style.height = "";
    panel.style.height = pyNow(panelH);
    box.style.top = pyNow(boxTop);
    bed.style.left = px(matrixBox.x);
    bed.style.top = pyNow(matrixBox.y);
    bed.style.width = px(matrixBox.w);
    bed.style.height = pyNow(matrixBox.h);
    document.querySelectorAll(".encoder").forEach((el, i) => {
      el.style.top = pyNow(encY);
      el.style.left = px(encX[i]);
    });
    t1.style.top = pyNow(padY);
    t2.style.top = pyNow(padY);
    t1.style.left = px(t1x);
    t2.style.left = px(t2x);
    t3.style.left = px(t3x);
    t3.style.top = "";
    document.querySelectorAll(".io--l, .io--r").forEach((el) => {
      const base = Number(el.dataset.y);
      el.style.top = pyNow(base + shift);
    });
    document.querySelectorAll(".voice").forEach((btn) => {
      const y = Number(btn.dataset.ch) + 1;
      const top = matrixBox.y + (16 - y + 0.5) * (matrixBox.h / 16);
      btn.style.top = pyNow(top);
    });
  }
  if (!midiAssign.hidden) layoutMidiAssign();
  syncBuildVersion(wide);
  const cur = device.cursor();
  crosshair.classList.toggle("on", assistEnabled && cur.on && !window.matchMedia("(prefers-reduced-motion: reduce)").matches);
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
function pauseIfInactive() {
  if (device.playing()) device.pause();
}
document.addEventListener("visibilitychange", () => {
  if (document.hidden) pauseIfInactive();
});
window.addEventListener("pagehide", pauseIfInactive);
window.addEventListener("blur", pauseIfInactive);
setDeviceFocused(true);
canvas.focus({ preventScroll: true });
markVoice();

const WELCOME_KEY = "toern-web-welcome-hide";
const welcomeModal = document.querySelector("#welcome-modal");
const welcomeDismiss = document.querySelector("#welcome-dismiss");
const welcomeHide = document.querySelector("#welcome-hide");
const welcomeOpen = document.querySelector("#welcome-open");
let welcomePrevFocus = null;

function openWelcome() {
  welcomePrevFocus = document.activeElement;
  welcomeModal.hidden = false;
  welcomeDismiss?.focus();
}

function closeWelcome() {
  if (welcomeHide?.checked) localStorage.setItem(WELCOME_KEY, "1");
  welcomeModal.hidden = true;
  (welcomePrevFocus || canvas)?.focus?.({ preventScroll: true });
}

welcomeDismiss?.addEventListener("click", closeWelcome);
welcomeOpen?.addEventListener("click", openWelcome);
welcomeModal?.addEventListener("click", (e) => {
  if (e.target === welcomeModal) closeWelcome();
});
window.addEventListener("keydown", (e) => {
  if (e.key === "Escape" && welcomeModal && !welcomeModal.hidden) {
    e.preventDefault();
    e.stopPropagation();
    closeWelcome();
  }
}, true);

const ENTRY_KEY = "toern-web-entry";
const entryGate = document.querySelector("#entry-gate");
const rotateGate = document.querySelector("#rotate-gate");
let welcomeAfterRotate = false;

function isHandheld() {
  const ua = navigator.userAgent || "";
  const ipad = /iPad/.test(ua) || (navigator.platform === "MacIntel" && navigator.maxTouchPoints > 1);
  return ipad || /iPhone|iPod|Android|webOS|BlackBerry|IEMobile|Opera Mini/i.test(ua);
}

function isLandscape() {
  return window.innerWidth >= window.innerHeight;
}

function hashParts() {
  return (location.hash || "").replace(/^#/, "").split(/[&/]/).filter(Boolean);
}

function hashWithoutMobile() {
  return hashParts().filter((part) => part.split("=")[0].trim().toLowerCase() !== "mobile").join("&");
}

function hashWithMobile() {
  const rest = hashWithoutMobile();
  return rest ? `${rest}&mobile` : "mobile";
}

function maybeOpenWelcome() {
  if (!welcomeModal || welcomeModal.hidden === false) return;
  if (localStorage.getItem(WELCOME_KEY) === "1") return;
  if (rotateGate && !rotateGate.hidden) return;
  if (entryGate && !entryGate.hidden) return;
  openWelcome();
}

function syncRotateGate() {
  const show = document.documentElement.classList.contains("mobile-ui")
    && entryGate?.hidden !== false
    && !isLandscape();
  if (rotateGate) rotateGate.hidden = !show;
  document.documentElement.classList.toggle("rotate-gate-on", show);
  if (!show && welcomeAfterRotate) {
    welcomeAfterRotate = false;
    maybeOpenWelcome();
  }
}

function finishMobileChoice() {
  if (!isLandscape()) {
    welcomeAfterRotate = localStorage.getItem(WELCOME_KEY) !== "1";
    syncRotateGate();
    return;
  }
  syncRotateGate();
  maybeOpenWelcome();
}

document.querySelector("#entry-yes")?.addEventListener("click", () => {
  sessionStorage.setItem(ENTRY_KEY, "yes");
  entryGate.hidden = true;
  document.documentElement.classList.remove("entry-gate-on");
  const next = hashWithMobile();
  if ((location.hash || "").replace(/^#/, "") !== next) location.hash = next;
  setMobileUi(true).then(() => {
    markVoice();
    finishMobileChoice();
  });
});

document.querySelector("#entry-no")?.addEventListener("click", () => {
  sessionStorage.setItem(ENTRY_KEY, "no");
  entryGate.hidden = true;
  document.documentElement.classList.remove("entry-gate-on");
  const next = hashWithoutMobile();
  if ((location.hash || "").replace(/^#/, "") !== next) location.hash = next;
  else setMobileUi(false).then(() => markVoice());
  maybeOpenWelcome();
});

window.addEventListener("resize", syncRotateGate);
window.addEventListener("orientationchange", syncRotateGate);

if (isHandheld() && !sessionStorage.getItem(ENTRY_KEY)) {
  entryGate.hidden = false;
  document.documentElement.classList.add("entry-gate-on");
  document.querySelector("#entry-yes")?.focus();
} else if (document.documentElement.classList.contains("mobile-ui") && !isLandscape()) {
  welcomeAfterRotate = localStorage.getItem(WELCOME_KEY) !== "1";
  syncRotateGate();
} else {
  document.documentElement.classList.remove("entry-gate-on");
  maybeOpenWelcome();
}
