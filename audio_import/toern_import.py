#!/usr/bin/env python3
"""TŒRN audio -> tracker-quantized clean 5/6-sample-voice + CH11 vocal-melody pattern + samplepack importer (V19).

Input: local audio file or YouTube URL.
Output:
  * 256-step TŒRN pattern
  * samplepack <slot>/1.wav .. <slot>/8.wav extracted from the source track
  * serial upload to TŒRN
  * current pattern configured to use the uploaded pack and saved to <slot>.txt

Pipeline:
  yt-dlp/ffmpeg -> Demucs -> librosa drums/BPM -> Basic Pitch bass + filtered other + filtered vocals
  -> clean monophonic 5/6-voice extraction -> PUTPAT -> serial PUT -> IMPORTSAVE

Musical behavior keeps the proven V5/V6 note extraction, but quantizes like a tracker:
each detected quarter beat is a hard anchor and is subdivided into four straight 16th cells.
Swing/microtiming is intentionally removed. No musical MIDI transposition is applied.

The importer intentionally extracts sounds from the source track instead of regenerating
new ones. This preserves timbre, articulation and production much better than synthesis.
"""
from __future__ import annotations

import argparse
import binascii
import math
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import wave
from typing import Iterable

import numpy as np

librosa = None
pretty_midi = None
serial = None
list_ports = None
sf = None
signal = None

STEPS = 256
ROWS = 16
NOTE_BYTES = STEPS * ROWS * 4
PITCH_HDR = bytes([ord('T'), ord('P'), ord('I'), ord('T'), 1, 0x10, 0x00, 0])
PATTERN_BYTES = NOTE_BYTES + 2 + 4 + len(PITCH_HDR) + STEPS * ROWS
NO_PITCH = 0xFF

CH_BD = 1
CH_SNARE = 2
CH_HH = 3  # source classifier ID; remapped to CH5 for playback
CH_PERC_SOURCE = 16  # internal classifier ID; remapped to CH6 for playback
CH_BASS = 3
CH_LEAD = 4
CH_HH_OPTIONAL = 5
CH_PERC_OPTIONAL = 6
CH_VOCAL_SYNTH = 11
SAMPLE_ROOT = 36  # sampler registration root
NEUTRAL_STORED_PITCH = 72  # TŒRN converts stored midiPitch to sampler trigger pitch via midiPitch - 36

# Current firmware reserves 15 MiB across 9 sample slots (preview 0 + voices 1..8):
# 15*1024*1024/9 = 1,747,626 bytes per slot (~19.8 s at 44.1kHz mono PCM16).
# Serial PUT on stock firmware uses 8192-byte ACK windows. To avoid the observed
# long-transfer SD/USB failure around 80 KiB, generated import samples are kept
# below 60 KiB each. This is far below the actual RAM limit and is intentional.
TOERN_SLOT_BYTES = (15 * 1024 * 1024) // 9
WAV_SR = 44100
WAV_CHANNELS = 1
WAV_SAMPLE_WIDTH = 2
MAX_IMPORT_WAV_BYTES = 60 * 1024
WAV_HEADER_BYTES = 44
MAX_IMPORT_FRAMES = (MAX_IMPORT_WAV_BYTES - WAV_HEADER_BYTES) // WAV_SAMPLE_WIDTH
MAX_IMPORT_SECONDS = MAX_IMPORT_FRAMES / WAV_SR
SERIAL_PUT_BLOCK = 8192


def load_dependencies() -> None:
    global librosa, pretty_midi, serial, list_ports, sf, signal
    try:
        import librosa as _librosa
        import pretty_midi as _pretty_midi
        import serial as _serial
        import soundfile as _sf
        from serial.tools import list_ports as _list_ports
        from scipy import signal as _signal
    except ImportError as exc:
        raise RuntimeError(
            f"Missing Python package: {exc}. Install with: pip install -r requirements.txt"
        ) from exc
    librosa = _librosa
    pretty_midi = _pretty_midi
    serial = _serial
    list_ports = _list_ports
    sf = _sf
    signal = _signal


def run(cmd: list[str], *, cwd: Path | None = None) -> None:
    print("  $ " + " ".join(str(x) for x in cmd))
    p = subprocess.run(cmd, cwd=cwd)
    if p.returncode != 0:
        raise RuntimeError(f"Command failed ({p.returncode}): {' '.join(cmd)}")


def require_bin(name: str) -> str:
    p = shutil.which(name)
    if not p:
        raise RuntimeError(f"Required command not found: {name}")
    return p


def is_url(s: str) -> bool:
    return s.startswith("http://") or s.startswith("https://")


def prepare_audio(source: str, work: Path) -> Path:
    require_bin("ffmpeg")
    wav = work / "source.wav"
    if is_url(source):
        require_bin("yt-dlp")
        raw = work / "download.%(ext)s"
        run(["yt-dlp", "-x", "--audio-format", "wav", "-o", str(raw), source])
        candidates = sorted(work.glob("download.*"), key=lambda p: p.stat().st_mtime, reverse=True)
        if not candidates:
            raise RuntimeError("yt-dlp produced no audio file")
        inp = candidates[0]
    else:
        inp = Path(source).expanduser().resolve()
        if not inp.exists():
            raise FileNotFoundError(inp)
    run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(inp), "-ar", "44100", str(wav)])
    return wav


def run_demucs(wav: Path, work: Path) -> dict[str, Path]:
    out = work / "demucs"
    run([sys.executable, "-m", "demucs", "--name", "htdemucs", "--out", str(out), str(wav)])
    roots = list(out.glob("htdemucs/*"))
    if not roots:
        raise RuntimeError("Demucs output not found")
    root = roots[0]
    stems = {name: root / f"{name}.wav" for name in ("drums", "bass", "other", "vocals")}
    missing = [str(p) for p in stems.values() if not p.exists()]
    if missing:
        raise RuntimeError("Missing Demucs stems: " + ", ".join(missing))
    return stems


def estimate_bpm_section_and_grid(wav: Path, bars: int = 16):
    """Return detected BPM, section start and a hard 16th-note tracker grid.

    TŒRN has no microtiming/swing per step: it stores one event in one of 256 cells.
    Therefore we use detected quarter-note beats as musical anchors and subdivide each
    quarter into exactly four straight 16ths. This removes swing and local MIDI timing
    noise while preserving the musical beat/bar structure.
    """
    y, sr = librosa.load(wav, sr=22050, mono=True)
    hop = 512
    onset_env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=hop)
    tempo, beat_frames = librosa.beat.beat_track(onset_envelope=onset_env, sr=sr, hop_length=hop)
    bpm_detected = float(np.asarray(tempo).reshape(-1)[0])
    if not math.isfinite(bpm_detected) or bpm_detected < 40 or bpm_detected > 300:
        bpm_detected = 120.0

    beat_frames = np.asarray(beat_frames, dtype=int)
    beat_times = np.asarray(librosa.frames_to_time(beat_frames, sr=sr, hop_length=hop), dtype=float)
    beats_needed = bars * 4

    if len(beat_times) == 0:
        start = 0.0
        quarter = 60.0 / bpm_detected
        beat_times = start + np.arange(beats_needed + 1, dtype=float) * quarter
        best_i = 0
    else:
        best_i, best_score = 0, -1.0
        max_i = max(1, len(beat_frames) - beats_needed)
        for i in range(max_i):
            a = beat_frames[i]
            b = beat_frames[min(i + beats_needed, len(beat_frames) - 1)]
            score = float(np.sum(onset_env[a:b]))
            if score > best_score:
                best_score, best_i = score, i
        start = float(beat_times[best_i])

    # Build 65 quarter-beat anchors for a 16-bar/256-step pattern. Missing tail
    # beats are extrapolated using the median local beat interval, not note timing.
    local = list(beat_times[best_i:best_i + beats_needed + 1]) if len(beat_times) else []
    if not local:
        local = [start]
    intervals = np.diff(local)
    good = intervals[(intervals > 0.20) & (intervals < 2.0)]
    quarter = float(np.median(good)) if len(good) else 60.0 / bpm_detected
    while len(local) < beats_needed + 1:
        local.append(float(local[-1] + quarter))
    local = np.asarray(local[:beats_needed + 1], dtype=float)

    # Straight tracker grid: 4 equally spaced cells inside every detected quarter.
    step_times = []
    for i in range(beats_needed):
        a, b = float(local[i]), float(local[i + 1])
        if b <= a:
            b = a + quarter
        d = (b - a) / 4.0
        step_times.extend([a + d * q for q in range(4)])
    step_times.append(float(local[beats_needed]))  # right boundary for step 255 decisions
    return bpm_detected, start, np.asarray(step_times, dtype=float)


def step_from_time(t: float, step_times: np.ndarray) -> int:
    """Snap an onset to the nearest hard tracker cell.

    No swing, no microtiming, no fractional step survives. Quarter-beat phase comes
    from the audio beat tracker, which is much more robust than seconds*BPM math.
    """
    if len(step_times) < 2:
        return -1
    if t < step_times[0] - (step_times[1] - step_times[0]) * 0.5:
        return -1
    if t >= step_times[-1] + (step_times[-1] - step_times[-2]) * 0.5:
        return STEPS
    idx = int(np.searchsorted(step_times, t, side='left'))
    if idx <= 0:
        return 0
    if idx >= len(step_times):
        return STEPS - 1
    left = idx - 1
    right = idx
    chosen = left if abs(t - step_times[left]) <= abs(step_times[right] - t) else right
    return min(STEPS - 1, int(chosen))


def velocity_from_strength(v: float, vmax: float) -> int:
    if vmax <= 1e-9:
        return 100
    x = max(0.0, min(1.0, v / vmax))
    return int(round(55 + 72 * math.sqrt(x)))


def classify_drums(drum_wav: Path, start_s: float, bpm: float, step_times: np.ndarray):
    """Return drum events plus best source onset time for each class."""
    y, sr = librosa.load(drum_wav, sr=22050, mono=True)
    hop = 256
    onset_env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=hop)
    onset_frames = librosa.onset.onset_detect(
        onset_envelope=onset_env, sr=sr, hop_length=hop, backtrack=True,
        units="frames", delta=0.12, wait=2,
    )
    if len(onset_frames) == 0:
        return [], {}

    S = np.abs(librosa.stft(y, n_fft=2048, hop_length=hop))
    freqs = librosa.fft_frequencies(sr=sr, n_fft=2048)
    low_mask = (freqs >= 35) & (freqs < 180)
    mid_mask = (freqs >= 180) & (freqs < 3500)
    high_mask = (freqs >= 3500) & (freqs < 10000)
    strengths = onset_env[np.minimum(onset_frames, len(onset_env) - 1)]
    vmax = float(np.percentile(strengths, 95)) if len(strengths) else 1.0
    events, occupied = [], set()
    best: dict[int, tuple[float, float]] = {}

    for frame, strength in zip(onset_frames, strengths):
        t = float(librosa.frames_to_time(frame, sr=sr, hop_length=hop))
        step = step_from_time(t, step_times)
        if step < 0 or step >= STEPS:
            continue
        col = min(int(frame), S.shape[1] - 1)
        low = float(np.mean(S[low_mask, col])) + 1e-9
        mid = float(np.mean(S[mid_mask, col])) + 1e-9
        high = float(np.mean(S[high_mask, col])) + 1e-9
        total = low + mid + high
        lr, hr = low / total, high / total
        if lr > 0.48 and low > high * 1.7:
            ch = CH_BD
        elif hr > 0.50 or high > mid * 1.6:
            ch = CH_HH
        elif 0.24 <= lr <= 0.48 and hr < 0.22 and mid > high * 1.35:
            # Mid/low-heavy non-kick transient: typically tom, clap/body percussion.
            # Kept as an optional sixth voice so the default five-voice arrangement stays clean.
            ch = CH_PERC_SOURCE
        else:
            ch = CH_SNARE
        key = (step, ch)
        if key not in occupied:
            occupied.add(key)
            events.append((step, ch, velocity_from_strength(float(strength), vmax), NEUTRAL_STORED_PITCH))
        # Sample-picking score is intentionally separate from the sequencer event logic.
        # Prefer a strong, spectrally characteristic and locally isolated hit instead of
        # simply taking the first/strongest occurrence. This does not change pattern timing.
        class_conf = (lr if ch == CH_BD else hr if ch == CH_HH else (mid / total if ch == CH_PERC_SOURCE else max(0.0, 1.0 - abs(lr - hr))))
        prev_t = float(librosa.frames_to_time(onset_frames[max(0, np.where(onset_frames == frame)[0][0]-1)], sr=sr, hop_length=hop)) if len(onset_frames) > 1 else t - 1.0
        idx_cur = int(np.where(onset_frames == frame)[0][0])
        next_t = float(librosa.frames_to_time(onset_frames[min(len(onset_frames)-1, idx_cur+1)], sr=sr, hop_length=hop)) if len(onset_frames) > 1 else t + 1.0
        gap_before = max(0.0, t - prev_t)
        gap_after = max(0.0, next_t - t)
        isolation = min(1.0, (min(gap_before, gap_after) + 0.03) / 0.25)
        sample_score = float(strength) * (0.65 + 0.7 * class_conf) * (0.65 + 0.7 * isolation)
        old = best.get(ch)
        if old is None or sample_score > old[1]:
            best[ch] = (t, sample_score)
    return events, {ch: t for ch, (t, _s) in best.items()}


def basic_pitch_midi(audio: Path, out_dir: Path, tag: str) -> Path:
    target = out_dir / tag
    target.mkdir(parents=True, exist_ok=True)
    run(["basic-pitch", str(target), str(audio)])
    mids = sorted(list(target.glob("*.mid")) + list(target.glob("*.midi")))
    if not mids:
        raise RuntimeError(f"Basic Pitch created no MIDI for {audio.name}")
    return mids[0]


def midi_notes(midi_path: Path):
    pm = pretty_midi.PrettyMIDI(str(midi_path))
    notes = []
    for inst in pm.instruments:
        if not inst.is_drum:
            notes.extend(inst.notes)
    return sorted(notes, key=lambda n: (n.start, -n.velocity))


def section_notes(notes, start_s: float, bpm: float, step_times: np.ndarray | None = None):
    end_s = float(step_times[-1]) if step_times is not None and len(step_times) else start_s + STEPS / 4.0 * 60.0 / bpm
    return [n for n in notes if start_s <= float(n.start) < end_s]


def choose_root_note(notes, start_s: float, bpm: float):
    notes = section_notes(notes, start_s, bpm)
    if not notes:
        return None
    # Choose an actual note close to the weighted median pitch. This minimizes
    # the amount of varispeed pitch-shifting across the whole imported phrase.
    usable = [n for n in notes if (n.end - n.start) >= 0.09] or notes
    weights = np.array([max(1.0, float(n.velocity)) * math.sqrt(max(0.02, n.end - n.start)) for n in usable])
    pitches = np.array([float(n.pitch) for n in usable])
    order = np.argsort(pitches)
    p_sorted = pitches[order]
    w_sorted = weights[order]
    target = float(p_sorted[np.searchsorted(np.cumsum(w_sorted), 0.5 * np.sum(w_sorted))])
    def score(n):
        dur = min(1.5, max(0.02, n.end - n.start))
        quality = float(n.velocity) * math.sqrt(dur)
        return quality / (1.0 + 0.75 * abs(float(n.pitch) - target))
    return max(usable, key=score)


def notes_to_events(notes, channel: int, start_s: float, bpm: float, step_times: np.ndarray,
                    pitch_min: int = 0, pitch_max: int = 127,
                    source_root: int | None = None):
    """Convert transcription notes to TŒRN events.

    When source_root is supplied, remap pitch for a sample whose real recorded pitch
    is source_root but whose TŒRN sampler registration is fixed at MIDI 36.
    """
    by_step = {}
    for n in notes:
        step = step_from_time(float(n.start), step_times)
        if step < 0 or step >= STEPS:
            continue
        desired = int(np.clip(n.pitch, pitch_min, pitch_max))
        if source_root is not None:
            # TŒRN's current sample playback path converts stored midiPitch to the
            # sampler trigger pitch as: trigger = storedMidi - 36. The sampler itself
            # has the WAV registered at root 36. For a WAV whose real source pitch is
            # R and desired output pitch D, trigger must be 36 + D - R, therefore:
            # storedMidi = 72 + D - R.
            pitch = NEUTRAL_STORED_PITCH + desired - int(source_root)
        else:
            pitch = desired
        pitch = int(np.clip(pitch, 0, 127))
        vel = int(np.clip(n.velocity, 1, 127))
        evt = (step, channel, vel, pitch)
        old = by_step.get(step)
        if old is None or vel > old[2]:
            by_step[step] = evt
    return list(by_step.values())


def _midi_to_hz(midi_note: int) -> float:
    return 440.0 * (2.0 ** ((float(midi_note) - 69.0) / 12.0))


def _score_tonal_candidate(stem_y: np.ndarray, sr: int, note, nearby_starts: list[float]) -> tuple[float, dict]:
    """Score one source occurrence as a multisample candidate.

    This is only for choosing the WAV cut. It never changes detected MIDI notes,
    BPM or sequencer quantization.
    """
    nstart = float(note.start)
    nend = float(note.end)
    dur = max(0.03, nend - nstart)
    # Analyze the stable interior: skip the attack, avoid borrowing too much of the next note.
    a_s = nstart + min(0.035, dur * 0.18)
    b_s = min(nend, a_s + min(0.48, max(0.12, dur * 0.72)))
    a = max(0, int(a_s * sr))
    b = min(len(stem_y), int(b_s * sr))
    seg = np.asarray(stem_y[a:b], dtype=np.float32)
    if len(seg) < int(0.06 * sr):
        return -1e9, {"reason": "too_short"}

    rms = float(np.sqrt(np.mean(seg * seg)) + 1e-12)
    peak = float(np.max(np.abs(seg)) + 1e-12)
    crest = peak / rms

    # pYIN gives us actual tuning/stability for the specific occurrence.
    try:
        # librosa defaults to 2048-point analysis. Some candidate interiors are shorter,
        # which caused noisy warnings and unnecessary padding. Use the largest power-of-two
        # frame that actually fits the signal.
        # Bass-safe pYIN window: C1 needs ~1350 samples at 22.05 kHz for two periods.
        # Pad short candidate interiors rather than shrinking frame_length below that limit.
        frame_length = 2048
        analysis_seg = seg
        if len(analysis_seg) < frame_length:
            analysis_seg = np.pad(analysis_seg, (0, frame_length - len(analysis_seg)))
        hop_length = 256
        f0, voiced_flag, voiced_prob = librosa.pyin(
            analysis_seg, fmin=librosa.note_to_hz('C1'), fmax=librosa.note_to_hz('C7'),
            sr=sr, frame_length=frame_length, hop_length=hop_length,
        )
        valid = np.isfinite(f0)
        if np.any(valid):
            midi_f = 69.0 + 12.0 * np.log2(np.asarray(f0[valid]) / 440.0)
            cents = 100.0 * (midi_f - float(note.pitch))
            tuning_err = abs(float(np.median(cents)))
            pitch_jitter = float(np.median(np.abs(cents - np.median(cents))))
            vp = np.asarray(voiced_prob)[valid] if voiced_prob is not None else np.ones(np.sum(valid))
            voiced = float(np.mean(vp))
        else:
            tuning_err, pitch_jitter, voiced = 250.0, 250.0, 0.0
    except Exception:
        tuning_err, pitch_jitter, voiced = 180.0, 180.0, 0.0

    # Prefer isolated notes: less bleed from other notes in the stem.
    prev_gap = 1.0
    next_gap = 1.0
    for t in nearby_starts:
        if t < nstart:
            prev_gap = min(prev_gap, nstart - t)
        elif t > nstart:
            next_gap = min(next_gap, t - nstart)
            break
    isolation = min(1.0, min(prev_gap, next_gap) / 0.22)

    # Tonalness: lower spectral flatness tends to be cleaner for a pitched sample.
    try:
        n_fft = 1 << int(math.floor(math.log2(max(128, min(2048, len(seg))))))
        n_fft = min(n_fft, len(seg))
        flat = float(np.mean(librosa.feature.spectral_flatness(y=seg, n_fft=n_fft, hop_length=max(32, n_fft // 4))))
    except Exception:
        flat = 0.2
    tonalness = float(np.clip(1.0 - flat * 3.0, 0.0, 1.0))

    # Score strongly rewards tuning + stability, then clean level/isolation.
    score = (
        2.3 * voiced
        + 1.8 * tonalness
        + 1.2 * isolation
        + 0.55 * min(1.0, dur / 0.45)
        + 0.35 * min(1.0, float(note.velocity) / 100.0)
        + 0.25 * min(1.0, rms / 0.08)
        - 0.012 * min(200.0, tuning_err)
        - 0.010 * min(200.0, pitch_jitter)
        - 0.08 * max(0.0, crest - 8.0)
    )
    return float(score), {
        "score": float(score), "tuning_cents": float(tuning_err),
        "jitter_cents": float(pitch_jitter), "voiced": float(voiced),
        "isolation": float(isolation), "duration": float(dur),
        "rms": float(rms), "tonalness": float(tonalness),
    }


def choose_multisample_roots(notes, start_s: float, bpm: float, stem: Path, count: int = 2):
    """Pick the best *occurrences* for low/mid/high multisample roots.

    We still choose real detected MIDI notes from the song. The difference from V14 is
    that we no longer use the first/obvious occurrence of a pitch: every candidate in
    the section is scored for tuning accuracy, pitch stability, isolation and level.
    """
    usable = section_notes(notes, start_s, bpm)
    if not usable:
        return []
    usable = [n for n in usable if (n.end - n.start) >= 0.08] or usable

    y, sr = librosa.load(stem, sr=22050, mono=True)
    starts = sorted(float(n.start) for n in usable)
    pitches = np.asarray([float(n.pitch) for n in usable])
    if count <= 1:
        targets = [float(np.median(pitches))]
    elif count == 2:
        targets = list(np.quantile(pitches, [0.30, 0.70]))
    else:
        targets = list(np.quantile(pitches, np.linspace(0.20, 0.80, count)))

    scored = []
    for n in usable:
        q, detail = _score_tonal_candidate(y, sr, n, starts)
        scored.append((n, q, detail))

    roots = []
    used_ids = set()
    for target in targets[:count]:
        candidates = []
        for n, q, detail in scored:
            if id(n) in used_ids:
                continue
            distance = abs(float(n.pitch) - float(target))
            # Stay near the intended zone but let an exceptionally clean note win.
            zone_score = q - 0.23 * distance
            candidates.append((zone_score, n, detail))
        if not candidates:
            break
        candidates.sort(key=lambda x: x[0], reverse=True)
        _zone_score, best, detail = candidates[0]
        setattr(best, '_toern_sample_quality', detail)
        roots.append(best)
        used_ids.add(id(best))

    roots.sort(key=lambda n: n.pitch)
    return roots


def clean_monophonic_notes(notes, start_s: float, bpm: float,
                           pitch_min: int, pitch_max: int, step_times: np.ndarray):
    """Create one clear melodic line from Basic Pitch output.

    This intentionally changes only the *selection* of overlapping transcription
    candidates, not BPM or grid math. Same-pitch fragments that are really one
    sustained note are merged. Near-simultaneous competing pitches are reduced to
    the stronger/longer candidate because one TŒRN sample lane is monophonic.
    """
    seq = [n for n in section_notes(notes, start_s, bpm, step_times)
           if pitch_min <= int(n.pitch) <= pitch_max and (n.end - n.start) >= 0.055]
    seq.sort(key=lambda n: (float(n.start), -float(n.velocity), -float(n.end-n.start)))
    if not seq:
        return []

    step_s = 60.0 / float(bpm) / 4.0
    merged = []
    for n in seq:
        if not merged:
            merged.append(n)
            continue
        prev = merged[-1]
        gap = float(n.start) - float(prev.end)
        onset_gap = float(n.start) - float(prev.start)

        # Basic Pitch often splits one held note into adjacent fragments. Keep one trigger.
        if int(n.pitch) == int(prev.pitch) and gap <= max(0.09, step_s * 0.60):
            prev.end = max(float(prev.end), float(n.end))
            prev.velocity = max(int(prev.velocity), int(n.velocity))
            continue

        # If two pitches begin essentially together, choose the clearer candidate.
        if onset_gap < min(0.070, step_s * 0.55):
            ps = float(prev.velocity) * max(0.06, float(prev.end-prev.start))**0.5
            ns = float(n.velocity) * max(0.06, float(n.end-n.start))**0.5
            if ns > ps:
                merged[-1] = n
            continue

        # Enforce a monophonic line: a new real onset terminates the previous note.
        if float(n.start) < float(prev.end):
            prev.end = max(float(prev.start) + 0.03, float(n.start))
        merged.append(n)

    # One event per 16th for this single sample voice. If several transcription
    # candidates quantize to the same step, keep the musically strongest one.
    by_step = {}
    for n in merged:
        step = step_from_time(float(n.start), step_times)
        if not 0 <= step < STEPS:
            continue
        score = float(n.velocity) * max(0.04, float(n.end-n.start))**0.5
        old = by_step.get(step)
        if old is None or score > old[1]:
            by_step[step] = (n, score)

    selected = [by_step[k][0] for k in sorted(by_step)]
    # Final de-fragmentation after quantization: do not retrigger the same pitch
    # while the prior detected note is still sounding.
    out = []
    for n in selected:
        if out and int(n.pitch) == int(out[-1].pitch):
            if float(n.start) <= float(out[-1].end) + max(0.06, step_s * 0.40):
                out[-1].end = max(float(out[-1].end), float(n.end))
                out[-1].velocity = max(int(out[-1].velocity), int(n.velocity))
                continue
        out.append(n)
    return out


def monophonic_events(notes, channel: int, source_root: int, start_s: float, bpm: float,
                      pitch_min: int, pitch_max: int, step_times: np.ndarray):
    cleaned = clean_monophonic_notes(notes, start_s, bpm, pitch_min, pitch_max, step_times)
    events = []
    for n in cleaned:
        step = step_from_time(float(n.start), step_times)
        desired = int(np.clip(n.pitch, pitch_min, pitch_max))
        stored = int(np.clip(NEUTRAL_STORED_PITCH + desired - int(source_root), 0, 127))
        vel = int(np.clip(n.velocity, 1, 127))
        events.append((step, channel, vel, stored))
    return events, cleaned



def make_melody_analysis_stem(src: Path, out: Path, highpass_hz: float, lowpass_hz: float = 6500.0) -> Path:
    """Create a pitch-analysis-only stem with low-frequency bleed and percussion reduced.

    The source stem itself is kept untouched for sample extraction. This derivative is only
    fed into Basic Pitch, so cleaner filtering cannot alter the selected source timbre.
    """
    y, sr = librosa.load(src, sr=WAV_SR, mono=True)
    if len(y) < 64:
        sf.write(str(out), y, WAV_SR, subtype="PCM_16")
        return out
    # Keep sustained harmonic content and suppress drum/transient leakage first.
    harm = librosa.effects.harmonic(y, margin=2.5)
    nyq = sr * 0.5
    lo = max(20.0, float(highpass_hz)) / nyq
    hi = min(float(lowpass_hz), nyq * 0.95) / nyq
    if 0.0 < lo < hi < 1.0:
        sos = signal.butter(4, [lo, hi], btype="bandpass", output="sos")
        harm = signal.sosfiltfilt(sos, harm).astype(np.float32, copy=False)
    # Gentle normalization for transcription only.
    peak = float(np.max(np.abs(harm))) if len(harm) else 0.0
    if peak > 1e-8:
        harm = harm * min(1.0, 0.90 / peak)
    out.parent.mkdir(parents=True, exist_ok=True)
    sf.write(str(out), harm, WAV_SR, subtype="PCM_16")
    return out


def synth_monophonic_events(notes, channel: int, start_s: float, bpm: float,
                              pitch_min: int, pitch_max: int, step_times: np.ndarray):
    """Monophonic synth lane using preserved absolute MIDI pitch, no sample-root compensation."""
    cleaned = clean_monophonic_notes(notes, start_s, bpm, pitch_min, pitch_max, step_times)
    events = []
    for n in cleaned:
        step = step_from_time(float(n.start), step_times)
        if not 0 <= step < STEPS:
            continue
        midi_pitch = int(np.clip(int(n.pitch), pitch_min, pitch_max))
        vel = int(np.clip(int(n.velocity), 1, 127))
        events.append((step, channel, vel, midi_pitch))
    return events, cleaned


def _fade(y: np.ndarray, sr: int, fade_in_ms=2.0, fade_out_ms=12.0) -> np.ndarray:
    if len(y) == 0:
        return y
    fi = min(len(y), int(sr * fade_in_ms / 1000.0))
    fo = min(len(y), int(sr * fade_out_ms / 1000.0))
    if fi > 1:
        y[:fi] *= np.linspace(0.0, 1.0, fi, dtype=np.float32)
    if fo > 1:
        y[-fo:] *= np.linspace(1.0, 0.0, fo, dtype=np.float32)
    return y


def _write_pcm16_mono_wav(path: Path, samples: np.ndarray) -> None:
    """Write the simplest WAV TŒRN expects: RIFF PCM16 LE, mono, 44.1 kHz.

    Uses stdlib wave so the output is a canonical uncompressed PCM WAV with no
    metadata chunks that could confuse a minimalist embedded parser.
    """
    x = np.asarray(samples, dtype=np.float32).reshape(-1)
    if len(x) > MAX_IMPORT_FRAMES:
        x = x[:MAX_IMPORT_FRAMES]
    x = np.clip(x, -1.0, 1.0)
    pcm = np.round(x * 32767.0).astype('<i2', copy=False)
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), 'wb') as wf:
        wf.setnchannels(WAV_CHANNELS)
        wf.setsampwidth(WAV_SAMPLE_WIDTH)
        wf.setframerate(WAV_SR)
        wf.setcomptype('NONE', 'not compressed')
        wf.writeframes(pcm.tobytes(order='C'))


def extract_sample(stem: Path, out: Path, onset_s: float, duration_s: float,
                   pre_s: float = 0.006, peak=0.92) -> None:
    y, sr = librosa.load(stem, sr=WAV_SR, mono=True)
    # Hard cap for serial reliability. Actual TŒRN RAM slot is much larger.
    duration_s = min(float(duration_s), MAX_IMPORT_SECONDS)
    start = max(0, int((onset_s - pre_s) * sr))
    end = min(len(y), start + int(duration_s * sr))
    # Snap the cut to a nearby low-amplitude / zero-crossing point to avoid clicks.
    search = int(0.006 * sr)
    lo = max(1, start - search)
    hi = min(len(y) - 1, start + search)
    if hi > lo:
        seg = y[lo:hi]
        crossings = np.flatnonzero(np.signbit(seg[:-1]) != np.signbit(seg[1:]))
        if len(crossings):
            cand = lo + crossings
            start = int(cand[np.argmin(np.abs(cand - start))])
        else:
            start = int(lo + np.argmin(np.abs(seg)))
    end = min(len(y), start + int(duration_s * sr))
    clip = np.asarray(y[start:end], dtype=np.float32).copy()
    if len(clip) < 32:
        clip = np.zeros(min(MAX_IMPORT_FRAMES, int(0.05 * sr)), dtype=np.float32)
    # Remove only obvious leading/trailing digital silence, preserving attacks/tails.
    nz = np.flatnonzero(np.abs(clip) > 2e-4)
    if len(nz):
        a = max(0, int(nz[0]) - int(0.003 * sr))
        b = min(len(clip), int(nz[-1]) + int(0.025 * sr))
        clip = clip[a:b]
    clip = clip[:MAX_IMPORT_FRAMES]
    clip = _fade(clip, sr)
    mx = float(np.max(np.abs(clip))) if len(clip) else 0.0
    if mx > 1e-8:
        clip *= min(1.0, peak / mx)
    _write_pcm16_mono_wav(out, clip)


def write_empty_wav(path: Path) -> None:
    _write_pcm16_mono_wav(path, np.zeros(1024, dtype=np.float32))


def build_samplepack(stems: dict[str, Path], drum_best: dict[int, float],
                     bass_notes, other_notes,
                     start_s: float, bpm: float, step_times: np.ndarray, pack_dir: Path,
                     include_hihat: bool = True, include_perc: bool = False):
    """Build a deliberately minimal samplepack: one voice per musical role.

    Default five voices: CH1 kick, CH2 snare, CH3 bass, CH4 lead/keys, CH5 hi-hat.
    Optional sixth voice: CH6 tom/clap/percussion from the drum stem. Bass and lead
    still use exactly one carefully selected, well-tuned source occurrence each.
    """
    pack_dir.mkdir(parents=True, exist_ok=True)
    meta = {"roots": {}, "files": {}, "labels": {}, "quality": {}}

    # Drums: use the best isolated hit already selected by classify_drums().
    drum_defs = [(CH_BD, "Kick", 0.42), (CH_SNARE, "Snare", 0.58)]
    if include_hihat:
        drum_defs.append((CH_HH_OPTIONAL, "HiHat", 0.22))
    if include_perc:
        drum_defs.append((CH_PERC_OPTIONAL, "Perc/Tom", 0.42))
    for ch, label, dur in drum_defs:
        src_ch = CH_HH if label == "HiHat" else CH_PERC_SOURCE if label == "Perc/Tom" else ch
        out = pack_dir / f"{ch}.wav"
        if src_ch in drum_best:
            extract_sample(stems["drums"], out, drum_best[src_ch], dur)
        else:
            write_empty_wav(out)
        meta["files"][ch] = out
        meta["labels"][ch] = label

    def add_single_root(notes, stem_name, ch, label, pitch_min, pitch_max):
        cleaned = clean_monophonic_notes(notes, start_s, bpm, pitch_min, pitch_max, step_times)
        roots = choose_multisample_roots(cleaned, start_s, bpm, stems[stem_name], 1)
        out = pack_dir / f"{ch}.wav"
        if roots:
            n = roots[0]
            # Keep V6 short-sample behavior, but choose the best tuned occurrence.
            dur = min(0.64, max(0.34, float(n.end-n.start) + 0.10))
            extract_sample(stems[stem_name], out, float(n.start), dur, pre_s=0.010)
            meta["roots"][ch] = int(n.pitch)
            q = getattr(n, '_toern_sample_quality', {})
            meta["quality"][ch] = q
            cents = q.get("tuning_cents") if isinstance(q, dict) else None
            tune = f", tune {cents:.0f}c" if isinstance(cents, (int,float)) else ""
            meta["labels"][ch] = f"{label} root {int(n.pitch)}{tune}"
        else:
            write_empty_wav(out)
            meta["labels"][ch] = f"{label} unavailable"
        meta["files"][ch] = out
        return cleaned

    bass_clean = add_single_root(bass_notes, "bass", CH_BASS, "Bass", 24, 72)
    lead_clean = add_single_root(other_notes, "other", CH_LEAD, "Lead/Keys", 48, 96)

    for ch in range(1, 9):
        p = pack_dir / f"{ch}.wav"
        if not p.exists():
            write_empty_wav(p)
            meta["files"][ch] = p
            meta["labels"][ch] = "unused"
    meta["bass_clean"] = bass_clean
    meta["lead_clean"] = lead_clean
    return meta

def verify_samplepack(pack_dir: Path) -> None:
    for ch in range(1, 9):
        p = pack_dir / f"{ch}.wav"
        size = p.stat().st_size
        if size > MAX_IMPORT_WAV_BYTES:
            raise RuntimeError(
                f"Sample {p.name} is {size} bytes, importer limit is {MAX_IMPORT_WAV_BYTES} bytes"
            )
        if size > TOERN_SLOT_BYTES:
            raise RuntimeError(
                f"Sample {p.name} exceeds TŒRN voice slot RAM ({TOERN_SLOT_BYTES} bytes)"
            )
        with wave.open(str(p), 'rb') as wf:
            if wf.getnchannels() != WAV_CHANNELS:
                raise RuntimeError(f"Invalid sample {p.name}: expected mono")
            if wf.getsampwidth() != WAV_SAMPLE_WIDTH:
                raise RuntimeError(f"Invalid sample {p.name}: expected signed PCM16")
            if wf.getframerate() != WAV_SR:
                raise RuntimeError(f"Invalid sample {p.name}: expected 44100 Hz")
            if wf.getcomptype() != 'NONE':
                raise RuntimeError(f"Invalid sample {p.name}: expected uncompressed PCM")
            if wf.getnframes() <= 0:
                raise RuntimeError(f"Invalid empty WAV: {p}")
            duration = wf.getnframes() / float(WAV_SR)
        # Cross-check libsndfile's interpretation too.
        info = sf.info(str(p))
        if info.format != 'WAV' or info.subtype != 'PCM_16' or info.channels != 1 or info.samplerate != WAV_SR:
            raise RuntimeError(
                f"Invalid sample {p.name}: got {info.format}/{info.subtype}, "
                f"{info.samplerate}Hz/{info.channels}ch"
            )
        print(f"      {p.name}: {size/1024:.1f} KB, {duration:.3f}s, WAV PCM16 mono 44.1k")


def build_pattern(bpm: float, events: Iterable[tuple[int, int, int, int]]) -> bytes:
    note = np.zeros((STEPS, ROWS, 4), dtype=np.uint8)
    pitch = np.full((STEPS, ROWS), NO_PITCH, dtype=np.uint8)
    for step, ch, vel, midi_pitch in events:
        if not (0 <= step < STEPS and 1 <= ch <= ROWS):
            continue
        row = ch - 1
        note[step, row, 0] = ch
        note[step, row, 1] = int(np.clip(vel, 1, 127))
        note[step, row, 2] = 100
        note[step, row, 3] = 1  # unconditional legacy condition
        pitch[step, row] = midi_pitch if 0 <= midi_pitch <= 127 else NO_PITCH
    payload = bytearray(note.tobytes(order="C"))
    payload += b"\xFF\xFE"
    payload += struct.pack("<f", float(bpm))
    payload += PITCH_HDR
    payload += pitch.tobytes(order="C")
    if len(payload) != PATTERN_BYTES:
        raise AssertionError((len(payload), PATTERN_BYTES))
    return bytes(payload)


def read_line(ser, timeout: float = 8.0) -> str:
    old = ser.timeout
    ser.timeout = min(0.25, timeout)
    deadline = time.time() + timeout
    try:
        while time.time() < deadline:
            raw = ser.readline()
            if raw:
                line = raw.decode("utf-8", errors="replace").strip()
                if line:
                    return line
        raise TimeoutError("Serial response timeout")
    finally:
        ser.timeout = old


def candidate_ports(explicit: str | None) -> list[str]:
    if explicit:
        return [explicit]
    ports = list(list_ports.comports())
    ports.sort(key=lambda p: (0 if p.vid == 0x16C0 else 1, 0 if "usbmodem" in p.device else 1, p.device))
    return [p.device for p in ports]


def connect_toern(explicit: str | None):
    errors = []
    for port in candidate_ports(explicit):
        try:
            ser = serial.Serial(port, 115200, timeout=0.5, write_timeout=10)
            time.sleep(0.15)
            ser.reset_input_buffer()
            ser.write(b"PING\n")
            ser.flush()
            line = read_line(ser, 2.0)
            if line.startswith("OK TOERN SD"):
                print(f"TŒRN connected: {port} : {line}")
                return ser
            ser.close()
        except Exception as exc:
            errors.append(f"{port}: {exc}")
    raise RuntimeError("No TŒRN serial device found" + ("\n" + "\n".join(errors) if errors else ""))


def serial_put_file(ser, remote_path: str, local_path: Path, retries: int = 3) -> None:
    """Upload using TŒRN's canonical 8192-byte PUT framing.

    Current stock firmware explicitly waits for one complete 8192-byte block (or
    the final shorter block) before replying ACK. Waiting after 2048 bytes causes
    the exact timeout seen with v3.01f-T2.
    """
    data = local_path.read_bytes()
    crc = binascii.crc32(data) & 0xFFFFFFFF
    last_error = None
    for attempt in range(1, retries + 1):
        try:
            ser.reset_input_buffer()
            ser.write(f"PUT {remote_path} {len(data)} {crc:08X}\n".encode("ascii"))
            ser.flush()
            line = read_line(ser, 8.0)
            if line != "READY":
                raise RuntimeError(f"expected READY, got {line!r}")
            for off in range(0, len(data), SERIAL_PUT_BLOCK):
                block = data[off:off + SERIAL_PUT_BLOCK]
                ser.write(block)
                ser.flush()
                line = read_line(ser, 20.0)
                if line != "ACK":
                    raise RuntimeError(f"expected ACK at {off + len(block)}, got {line!r}")
            line = read_line(ser, 10.0)
            if line != "OK":
                raise RuntimeError(f"final response {line!r}")
            return
        except Exception as exc:
            last_error = exc
            if attempt >= retries:
                break
            print(f"        retry {attempt}/{retries - 1} after upload error: {exc}")
            time.sleep(0.5)
            ser.reset_input_buffer()
    raise RuntimeError(f"PUT {remote_path} failed after {retries} attempts: {last_error}")


def serial_put_pattern(ser, payload: bytes) -> None:
    crc = binascii.crc32(payload) & 0xFFFFFFFF
    ser.reset_input_buffer()
    ser.write(f"PUTPAT {len(payload)} {crc:08X}\n".encode("ascii"))
    ser.flush()
    line = read_line(ser)
    if line != "READY":
        raise RuntimeError(f"PUTPAT: expected READY, got {line!r}")
    for off in range(0, len(payload), SERIAL_PUT_BLOCK):
        ser.write(payload[off:off + SERIAL_PUT_BLOCK])
        ser.flush()
        line = read_line(ser, 12.0)
        if line != "ACK":
            raise RuntimeError(f"PUTPAT: expected ACK at {off}, got {line!r}")
    line = read_line(ser, 8.0)
    if line != "OK":
        raise RuntimeError(f"PUTPAT failed: {line}")


def serial_cmd_ok(ser, cmd: str, timeout=30.0) -> str:
    ser.write((cmd + "\n").encode("ascii"))
    ser.flush()
    line = read_line(ser, timeout)
    if not line.startswith("OK"):
        if "NEED_SD" in line or "UNKNOWN" in line:
            raise RuntimeError(
                f"Firmware does not support {cmd.split()[0]} yet. Apply the included V6 firmware patch and reflash."
            )
        raise RuntimeError(f"{cmd} failed: {line}")
    return line


def serial_mkdir(ser, remote_dir: str) -> None:
    ser.write((f"MKDIR {remote_dir}\n").encode("ascii"))
    ser.flush()
    line = read_line(ser, 10.0)
    upper = line.upper()
    if line.startswith("OK"):
        return
    if upper.startswith("ERR MKDIR") or "EXIST" in upper:
        print(f"      MKDIR /{remote_dir}: {line} (continuing)")
        return
    raise RuntimeError(f"MKDIR {remote_dir} failed: {line}")


def _read_exact(ser, size: int, timeout: float = 15.0) -> bytes:
    old = ser.timeout
    ser.timeout = 0.5
    out = bytearray()
    deadline = time.time() + timeout
    try:
        while len(out) < size:
            chunk = ser.read(size - len(out))
            if chunk:
                out += chunk
                deadline = time.time() + timeout
            elif time.time() > deadline:
                raise TimeoutError(f"Serial binary read timeout ({len(out)}/{size})")
        return bytes(out)
    finally:
        ser.timeout = old


def serial_get_file(ser, remote_path: str) -> bytes:
    ser.reset_input_buffer()
    ser.write(f"GET {remote_path}\n".encode("ascii"))
    ser.flush()
    line = read_line(ser, 10.0)
    if not line.startswith("OK "):
        raise RuntimeError(f"GET {remote_path} failed: {line}")
    try:
        size = int(line.split()[1])
    except Exception as exc:
        raise RuntimeError(f"GET {remote_path}: invalid size response {line!r}") from exc
    ser.write(b"ACK\n")
    ser.flush()
    data = bytearray()
    while len(data) < size:
        n = min(512, size - len(data))
        data += _read_exact(ser, n)
        ser.write(b"ACK\n")
        ser.flush()
    crc_line = read_line(ser, 10.0)
    if not crc_line.startswith("CRC "):
        raise RuntimeError(f"GET {remote_path}: expected CRC, got {crc_line!r}")
    remote_crc = int(crc_line.split()[1], 16)
    local_crc = binascii.crc32(data) & 0xFFFFFFFF
    if remote_crc != local_crc:
        raise RuntimeError(f"GET {remote_path}: CRC mismatch {remote_crc:08X} != {local_crc:08X}")
    return bytes(data)


def validate_saved_pattern(saved: bytes, putpat_payload: bytes, bpm: float) -> None:
    # Canonical .txt layout: notes + FF FE + raw SMP struct + TPIT header + pitch bytes.
    if len(saved) <= NOTE_BYTES + 2 + len(PITCH_HDR) + STEPS * ROWS:
        raise RuntimeError(f"Saved pattern is implausibly short: {len(saved)} bytes")
    if saved[:NOTE_BYTES] != putpat_payload[:NOTE_BYTES]:
        raise RuntimeError("Saved 29.txt note matrix does not match uploaded pattern")
    if saved[NOTE_BYTES:NOTE_BYTES + 2] != b"\xFF\xFE":
        raise RuntimeError("Saved pattern is missing FF FE SMP marker")
    pitch_pos = len(saved) - (len(PITCH_HDR) + STEPS * ROWS)
    if saved[pitch_pos:pitch_pos + len(PITCH_HDR)] != PITCH_HDR:
        raise RuntimeError("Saved pattern is missing TPIT pitch extension")
    if saved[pitch_pos + len(PITCH_HDR):] != putpat_payload[-STEPS * ROWS:]:
        raise RuntimeError("Saved pattern MIDI pitches do not match uploaded pattern")
    saved_bpm = struct.unpack_from("<f", saved, NOTE_BYTES + 2)[0]
    if abs(saved_bpm - bpm) > 0.05:
        raise RuntimeError(f"Saved pattern BPM mismatch: {saved_bpm:.2f} != {bpm:.2f}")


def count_by_channel(events) -> dict[int, int]:
    d = {}
    for _, ch, _, _ in events:
        d[ch] = d.get(ch, 0) + 1
    return d


def output_root_for_source(source: str | None) -> Path:
    if not source or is_url(source):
        return Path.cwd()
    return Path(source).expanduser().resolve().parent


def main() -> int:
    ap = argparse.ArgumentParser(description="Convert audio/YouTube to a clean TŒRN pattern: kick, snare, hi-hat, monophonic bass, monophonic lead/keys, optional percussion")
    ap.add_argument("source", nargs="?", help="MP3/WAV/etc or YouTube URL; optional with --upload")
    ap.add_argument("--slot", type=int, default=29, help="Pattern and samplepack slot, default: 29")
    ap.add_argument("--port", help="Serial port, default: auto-detect")
    ap.add_argument("--start", type=float, help="Section start seconds, default: densest 16 bars")
    ap.add_argument("--keep", action="store_true", help="Keep temporary analysis folder")
    ap.add_argument("--no-send", action="store_true", help="Build files only")
    ap.add_argument("--upload", action="store_true", help="Upload the last generated local pattern/samplepack; skip analysis")
    ap.add_argument("--voices", type=int, choices=[5,6], default=5, help="5 = kick/snare/hi-hat/bass/lead (default); 6 also adds one clean tom/clap/percussion voice")
    args = ap.parse_args()
    if not (1 <= args.slot <= 999):
        ap.error("--slot must be 1..999")
    if not args.upload and not args.source:
        ap.error("source is required unless --upload is used")
    load_dependencies()

    if args.upload:
        preview = Path.cwd() / f"toern_pattern_{args.slot}.bin"
        pack_dir = Path.cwd() / f"toern_samplepack_{args.slot}"
        if not preview.exists():
            raise RuntimeError(f"Missing local pattern: {preview}")
        for ch in range(1, 9):
            if not (pack_dir / f"{ch}.wav").exists():
                raise RuntimeError(f"Missing local sample: {pack_dir / f'{ch}.wav'}")
        payload = preview.read_bytes()
        if len(payload) != PATTERN_BYTES:
            raise RuntimeError(f"Local pattern has unexpected size {len(payload)} != {PATTERN_BYTES}")
        bpm = struct.unpack_from("<f", payload, NOTE_BYTES + 2)[0]
        print(f"UPLOAD ONLY : using {preview.name} + {pack_dir.name}, BPM {bpm:.2f}")
        ser = connect_toern(args.port)
        try:
            print(f"      PUTPAT first ({len(payload)} bytes)")
            serial_put_pattern(ser, payload)
            print("      PUTPAT: OK")
            serial_mkdir(ser, str(args.slot))
            for ch in range(1, 9):
                local = pack_dir / f"{ch}.wav"
                remote = f"{args.slot}/{ch}.wav"
                print(f"      PUT /{remote} ({local.stat().st_size // 1024} KB)")
                serial_put_file(ser, remote, local)
            line = serial_cmd_ok(ser, f"IMPORTSAVE {args.slot}", timeout=120.0)
            print(f"      IMPORTSAVE {args.slot}: {line}")
            saved = serial_get_file(ser, f"{args.slot}.txt")
            validate_saved_pattern(saved, payload, bpm)
            (Path.cwd() / f"toern_pattern_{args.slot}.txt").write_bytes(saved)
            canonical = output_root_for_source(args.source) / f"{args.slot}.txt"
            canonical.write_bytes(saved)
            print(f"      GET /{args.slot}.txt: verified ({len(saved)} bytes, CRC OK)")
            print(f"      Local canonical copy: {canonical}")
        finally:
            ser.close()
        print(f"DONE : uploaded slot {args.slot}")
        return 0

    temp_ctx = tempfile.TemporaryDirectory(prefix="toern-import-")
    work = Path(temp_ctx.name)
    try:
        print("[1/9] Preparing audio")
        wav = prepare_audio(args.source, work)
        print("[2/9] Separating stems with Demucs")
        stems = run_demucs(wav, work)
        # Keep raw stems accessible next to the source instead of losing them with /tmp.
        out_root = output_root_for_source(args.source)
        stems_out = out_root / "stems"
        stems_out.mkdir(parents=True, exist_ok=True)
        for stem_name, stem_path in stems.items():
            shutil.copy2(stem_path, stems_out / f"{stem_name}.wav")
        print(f"      Persistent stems: {stems_out}")
        print("[3/9] Detecting BPM and tracker grid")
        bpm_detected, auto_start, step_times = estimate_bpm_section_and_grid(wav)
        bpm = float(round(bpm_detected))
        start_s = float(args.start) if args.start is not None else auto_start
        print(f"      BPM detected: {bpm_detected:.2f}")
        print(f"      TŒRN BPM: {int(bpm)}")
        print(f"      Section start: {start_s:.2f}s")
        print(f"      Tracker grid: 256 hard 16th cells, beat-anchored, swing/microtiming removed")

        print("[4/9] Detecting kick / snare / hi-hat")
        drum_events, drum_best = classify_drums(stems["drums"], start_s, bpm, step_times)

        print("[5/9] Transcribing bass, filtered lead/keys and vocal melody with Basic Pitch")
        midi_dir = work / "midi"
        analysis_dir = work / "melody_analysis"
        analysis_dir.mkdir(parents=True, exist_ok=True)
        # Bass keeps its own separated stem. For melody lanes, remove low-frequency bleed
        # and transient residue before transcription. Raw stems remain untouched for samples.
        other_melody = make_melody_analysis_stem(stems["other"], analysis_dir / "other_melody.wav", 180.0, 6500.0)
        vocal_melody = make_melody_analysis_stem(stems["vocals"], analysis_dir / "vocals_melody.wav", 140.0, 5200.0)
        shutil.copy2(other_melody, stems_out / "other_melody_filtered.wav")
        shutil.copy2(vocal_melody, stems_out / "vocals_melody_filtered.wav")
        bass_notes = midi_notes(basic_pitch_midi(stems["bass"], midi_dir, "bass"))
        other_notes = midi_notes(basic_pitch_midi(other_melody, midi_dir, "other_melody"))
        vocal_notes = midi_notes(basic_pitch_midi(vocal_melody, midi_dir, "vocals_melody"))

        print("[6/9] Extracting short source-derived TŒRN samplepack")
        pack_dir = Path.cwd() / f"toern_samplepack_{args.slot}"
        if pack_dir.exists():
            shutil.rmtree(pack_dir)
        meta = build_samplepack(stems, drum_best, bass_notes, other_notes, start_s, bpm, step_times, pack_dir, include_hihat=True, include_perc=(args.voices >= 6))
        verify_samplepack(pack_dir)
        stem_map = {1:"drums", 2:"drums", 3:"bass", 4:"other", 5:"drums", 6:"drums", 7:"unused", 8:"unused"}
        name_lines = []
        for ch in range(1, 9):
            root = meta["roots"].get(ch)
            suffix = f", root MIDI {root}" if root is not None else ""
            label = meta['labels'].get(ch, 'sample')
            print(f"      /{args.slot}/{ch}.wav  {label}{suffix}")
            name_lines.append(f"{ch}_{stem_map.get(ch,'unknown')}_{label}" + (f" | root MIDI {root}" if root is not None else ""))
        name_lines.append("11_vocals_Vocal melody -> CH11 poly synth")
        (stems_out / "stem_names.txt").write_text("\n".join(name_lines) + "\n", encoding="utf-8")

        # Deliberately minimal arrangement: one monophonic lane per stem/role.
        allowed_drum_ids = (CH_BD, CH_SNARE, CH_HH, CH_PERC_SOURCE) if args.voices >= 6 else (CH_BD, CH_SNARE, CH_HH)
        drum_events = [e for e in drum_events if e[1] in allowed_drum_ids]
        drum_events = [
            (st, CH_HH_OPTIONAL if ch == CH_HH else CH_PERC_OPTIONAL if ch == CH_PERC_SOURCE else ch, vel, pit)
            for st, ch, vel, pit in drum_events
        ]
        bass_root = meta["roots"].get(CH_BASS)
        lead_root = meta["roots"].get(CH_LEAD)
        bass_events = []
        lead_events = []
        if bass_root is not None:
            bass_events, _ = monophonic_events(meta["bass_clean"], CH_BASS, bass_root, start_s, bpm, 24, 72, step_times)
        if lead_root is not None:
            lead_events, _ = monophonic_events(meta["lead_clean"], CH_LEAD, lead_root, start_s, bpm, 48, 96, step_times)
        vocal_events, vocal_clean = synth_monophonic_events(vocal_notes, CH_VOCAL_SYNTH, start_s, bpm, 48, 96, step_times)
        events = drum_events + bass_events + lead_events + vocal_events
        if not events:
            raise RuntimeError("No usable musical events detected in the selected section; refusing to save an empty TŒRN slot")

        print("[7/9] Building TŒRN pattern")
        payload = build_pattern(bpm, events)
        preview = Path.cwd() / f"toern_pattern_{args.slot}.bin"
        preview.write_bytes(payload)
        counts = count_by_channel(events)
        for ch in range(1, 9):
            if counts.get(ch):
                print(f"      CH{ch:02d} {meta['labels'].get(ch, 'SAMPLE'):18s}: {counts[ch]} events")
        if counts.get(CH_VOCAL_SYNTH):
            print(f"      CH11 Vocal melody -> synth : {counts[CH_VOCAL_SYNTH]} events")

        if args.no_send:
            print("[8/9] --no-send : samplepack and pattern built locally")
            print(f"[9/9] DONE : {preview} + {pack_dir}")
            return 0

        print("[8/9] Connecting and uploading generated slot")
        ser = connect_toern(args.port)
        try:
            print(f"      PUTPAT first ({len(payload)} bytes, {SERIAL_PUT_BLOCK}-byte ACK framing)")
            serial_put_pattern(ser, payload)
            print("      PUTPAT: OK")
            serial_mkdir(ser, str(args.slot))
            for ch in range(1, 9):
                local = pack_dir / f"{ch}.wav"
                remote = f"{args.slot}/{ch}.wav"
                print(f"      PUT /{remote} ({local.stat().st_size // 1024} KB)")
                serial_put_file(ser, remote, local)

            print("[9/9] Saving canonical pattern + settings, reloading and verifying")
            line = serial_cmd_ok(ser, f"IMPORTSAVE {args.slot}", timeout=120.0)
            print(f"      IMPORTSAVE {args.slot}: {line}")
            saved = serial_get_file(ser, f"{args.slot}.txt")
            validate_saved_pattern(saved, payload, bpm)
            saved_copy = Path.cwd() / f"toern_pattern_{args.slot}.txt"
            saved_copy.write_bytes(saved)
            canonical_copy = output_root_for_source(args.source) / f"{args.slot}.txt"
            canonical_copy.write_bytes(saved)
            print(f"      GET /{args.slot}.txt: verified ({len(saved)} bytes, CRC OK)")
            print(f"      Local canonical copy: {canonical_copy}")
            print("      Canonical reload on TŒRN: OK (pattern + SMP settings + samplepack)")
        finally:
            ser.close()

        print(f"DONE : verified pattern {args.slot}.txt + samplepack /{args.slot}, BPM {bpm:.2f}, 256 steps")
        return 0
    finally:
        if args.keep:
            keep = Path.cwd() / "toern_import_debug"
            if keep.exists():
                shutil.rmtree(keep)
            shutil.copytree(work, keep)
            print(f"Debug files copied to: {keep}")
        temp_ctx.cleanup()

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\nCancelled", file=sys.stderr)
        raise SystemExit(130)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
