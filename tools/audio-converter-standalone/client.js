import { FFmpeg } from "./classes.js";
import JSZip from "./jszip.mjs";

async function fetchFile(file) {
  if (file instanceof File || file instanceof Blob) {
    return new Uint8Array(await file.arrayBuffer());
  }
  if (typeof file === "string" || file instanceof URL) {
    return new Uint8Array(await (await fetch(file)).arrayBuffer());
  }
  return new Uint8Array();
}

const form = document.getElementById("converter-form");
const folderInput = document.getElementById("folderInput");
const zipInput = document.getElementById("zipInput");
const submitBtn = document.getElementById("submitBtn");
const statusEl = document.getElementById("status");
const progressLabel = document.getElementById("progressLabel");
const progressPercent = document.getElementById("progressPercent");
const progressFill = document.getElementById("progressFill");
const progressBarWrap = document.getElementById("progressBarWrap");

const AUDIO_EXTENSIONS = new Set([
  ".aac", ".ac3", ".aif", ".aiff", ".aifc", ".alac", ".amr", ".ape",
  ".au", ".caf", ".dts", ".flac", ".m4a", ".m4b", ".m4p", ".mka",
  ".mp1", ".mp2", ".mp3", ".oga", ".ogg", ".opus", ".ra", ".rm",
  ".snd", ".spx", ".tta", ".wav", ".webm", ".wma", ".wv", ".mid",
  ".midi", ".3gp", ".3g2",
]);

let ffmpegInstance = null;
let ffmpegLoading = null;

function setStatus(message, variant = "") {
  statusEl.textContent = message;
  statusEl.classList.remove("is-error", "is-success");
  if (variant) statusEl.classList.add(variant);
}

function updateProgress(value, label) {
  const clamped = Math.max(0, Math.min(100, Math.round(value)));
  progressFill.style.width = `${clamped}%`;
  progressPercent.textContent = `${clamped}%`;
  progressBarWrap.setAttribute("aria-valuenow", String(clamped));
  if (label) progressLabel.textContent = label;
}

function inferOutputName(uploadName) {
  if (!uploadName) return "converted_audio.zip";
  const noZip = uploadName.replace(/\.zip$/i, "");
  const safe = noZip.trim() || "converted_audio";
  return `${safe}_converted.zip`;
}

function sanitizeSegment(input) {
  if (!input) return "unnamed";

  let value = input
    .normalize("NFKD")
    .replace(/[\u0300-\u036f]/g, "")
    .replace(/[^\x20-\x7E]/g, "-")
    .replace(/[\\/:*?"<>|]/g, "-")
    .replace(/\s+/g, " ")
    .trim()
    .replace(/-+/g, "-");

  value = value.replace(/^\.+/, "").replace(/\.+$/, "").trim();
  if (!value) return "unnamed";
  return value.slice(0, 120);
}

function sanitizeRelativePath(inputPath) {
  if (!inputPath || typeof inputPath !== "string") return null;

  const parts = inputPath
    .split(/[\\/]+/)
    .filter((part) => part && part !== "." && part !== "..");

  if (!parts.length) return null;
  return parts.map((part) => sanitizeSegment(part)).join("/");
}

function isAudioPath(filePath) {
  const dot = filePath.lastIndexOf(".");
  if (dot < 0) return false;
  return AUDIO_EXTENSIONS.has(filePath.slice(dot).toLowerCase());
}

function outputWavPath(inputPath) {
  const safePath = sanitizeRelativePath(inputPath);
  if (!safePath) return null;

  const slash = safePath.lastIndexOf("/");
  const dir = slash >= 0 ? safePath.slice(0, slash) : "";
  const filename = slash >= 0 ? safePath.slice(slash + 1) : safePath;
  const dot = filename.lastIndexOf(".");
  const base = dot >= 0 ? filename.slice(0, dot) : filename;
  const wavName = `${base}.wav`;
  return dir ? `${dir}/${wavName}` : wavName;
}

function vfsName(prefix, index, extension) {
  const safeExt = (extension || "bin").replace(/[^a-zA-Z0-9]/g, "") || "bin";
  return `${prefix}_${index}.${safeExt}`;
}

async function getFfmpeg() {
  if (ffmpegInstance) {
    return ffmpegInstance;
  }

  if (!ffmpegLoading) {
    ffmpegLoading = (async () => {
      const ffmpeg = new FFmpeg();
      await ffmpeg.load({
        classWorkerURL: new URL("./worker.js", import.meta.url).href,
        coreURL: new URL("./ffmpeg-core.js", import.meta.url).href,
        wasmURL: new URL("./ffmpeg-core.wasm", import.meta.url).href,
      });
      ffmpegInstance = ffmpeg;
      return ffmpeg;
    })();
  }

  return ffmpegLoading;
}

async function collectFolderFiles(fileList) {
  const entries = [];

  for (const file of fileList) {
    const relativePath = sanitizeRelativePath(file.webkitRelativePath || file.name);
    if (!relativePath || !isAudioPath(relativePath)) continue;
    entries.push({ relativePath, data: await fetchFile(file) });
  }

  return entries;
}

async function collectZipFiles(zipFile) {
  const zip = await JSZip.loadAsync(zipFile);
  const entries = [];

  for (const [relativePath, zipEntry] of Object.entries(zip.files)) {
    if (zipEntry.dir) continue;

    const safePath = sanitizeRelativePath(relativePath);
    if (!safePath || !isAudioPath(safePath)) continue;

    const blob = await zipEntry.async("blob");
    entries.push({ relativePath: safePath, data: await fetchFile(blob) });
  }

  return entries;
}

async function convertToWav(ffmpeg, index, relativePath, inputData) {
  const inputExt = relativePath.includes(".")
    ? relativePath.slice(relativePath.lastIndexOf(".") + 1)
    : "bin";
  const inputName = vfsName("in", index, inputExt);
  const outputName = vfsName("out", index, "wav");

  await ffmpeg.writeFile(inputName, inputData);
  try {
    await ffmpeg.exec([
      "-i", inputName,
      "-ac", "1",
      "-ar", "44100",
      "-acodec", "pcm_s16le",
      "-f", "wav",
      outputName,
    ]);
    return await ffmpeg.readFile(outputName);
  } finally {
    try { await ffmpeg.deleteFile(inputName); } catch { /* ignore */ }
    try { await ffmpeg.deleteFile(outputName); } catch { /* ignore */ }
  }
}

function triggerDownload(blob, filename) {
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = filename;
  document.body.appendChild(link);
  link.click();
  link.remove();
  URL.revokeObjectURL(url);
}

folderInput.addEventListener("change", () => {
  if (folderInput.files.length > 0) {
    zipInput.value = "";
    setStatus(`Folder selected: ${folderInput.files.length} file(s).`);
  }
});

zipInput.addEventListener("change", () => {
  if (zipInput.files.length > 0) {
    folderInput.value = "";
    setStatus(`ZIP selected: ${zipInput.files[0].name}.`);
  }
});

form.addEventListener("submit", async (event) => {
  event.preventDefault();

  const hasFolderFiles = folderInput.files.length > 0;
  const hasZip = zipInput.files.length > 0;

  if (!hasFolderFiles && !hasZip) {
    setStatus("Please select a folder or a ZIP file.", "is-error");
    return;
  }

  let sourceName = "converted_audio";
  submitBtn.disabled = true;

  try {
    updateProgress(2, "Loading converter");
    setStatus("Loading converter (first run may take a moment)...", "");

    const ffmpeg = await getFfmpeg();

    updateProgress(8, "Reading files");
    setStatus("Reading uploaded files...", "");

    const audioFiles = hasZip
      ? await collectZipFiles(zipInput.files[0])
      : await collectFolderFiles(folderInput.files);

    if (hasZip) {
      sourceName = zipInput.files[0].name;
    } else {
      const firstPath = folderInput.files[0]?.webkitRelativePath || "";
      sourceName = firstPath.split("/")[0] || "folder";
    }

    if (audioFiles.length === 0) {
      throw new Error("No audio files found. Supported formats include WAV, MP3, FLAC, M4A, OGG, and more.");
    }

    const outputZip = new JSZip();

    for (let index = 0; index < audioFiles.length; index += 1) {
      const { relativePath, data } = audioFiles[index];
      const outputPath = outputWavPath(relativePath);
      if (!outputPath) continue;

      const fileProgress = 10 + Math.round((index / audioFiles.length) * 80);
      updateProgress(fileProgress, `Converting ${index + 1}/${audioFiles.length}`);
      setStatus(`Converting ${relativePath}...`, "");

      const wavData = await convertToWav(ffmpeg, index, relativePath, data);
      outputZip.file(outputPath, wavData);
    }

    updateProgress(95, "Creating ZIP");
    setStatus("Creating download ZIP...", "");

    const zipBlob = await outputZip.generateAsync({
      type: "blob",
      compression: "DEFLATE",
      compressionOptions: { level: 9 },
    });

    updateProgress(99, "Downloading");
    triggerDownload(zipBlob, inferOutputName(sourceName));

    updateProgress(100, "Done");
    setStatus("Conversion successful. ZIP download started.", "is-success");
  } catch (error) {
    updateProgress(0, "Failed");
    setStatus(error?.message || "Conversion failed.", "is-error");
  } finally {
    submitBtn.disabled = false;
  }
});
