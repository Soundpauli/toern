"use strict";

const fs = require("fs");
const path = require("path");

try {
  const ffmpegPath = require("ffmpeg-static");
  if (ffmpegPath && fs.existsSync(ffmpegPath)) {
    fs.chmodSync(ffmpegPath, 0o755);
  }
} catch {
  // ffmpeg-static is optional during partial installs.
}
