#!/usr/bin/env python3
"""TŒRN audio analysis for the simulator importer.

Does only the heavy, non-interactive work and leaves every timing decision to the
simulator (tempo fit, start beat, grid nudge, per-lane offsets, quantization):

  source -> Demucs stems -> drum onsets (attack-refined, classified)
         -> Basic Pitch bass / filtered other / filtered vocals (onset-refined)
         -> source-derived samplepack 1..8.wav
         -> analysis.json with raw event times in seconds

Usage: python toern_analyze.py SOURCE --out DIR
Intermediate results (stems, MIDI) are cached in DIR, so re-running is cheap.
Progress is reported as lines: "@@PROGRESS <percent> <message>".
"""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys

import numpy as np

import toern_import as ti

ANALYSIS_VERSION = 4


def progress(pct: int, msg: str) -> None:
    print(f"@@PROGRESS {pct} {msg}", flush=True)


def run_demucs_cached(wav: Path, out: Path) -> dict[str, Path]:
    stems_dir = out / "stems"
    names = ("drums", "bass", "other", "vocals")
    stems = {n: stems_dir / f"{n}.wav" for n in names}
    if all(p.exists() for p in stems.values()):
        return stems
    work = out / "demucs_work"
    for device in ("mps", "cpu"):
        cmd = [sys.executable, "-m", "demucs", "--name", "htdemucs", "-d", device, "--out", str(work), str(wav)]
        print("  $ " + " ".join(cmd), flush=True)
        if subprocess.run(cmd).returncode == 0:
            break
    else:
        raise RuntimeError("Demucs failed")
    roots = list(work.glob("htdemucs/*"))
    if not roots:
        raise RuntimeError("Demucs output not found")
    stems_dir.mkdir(parents=True, exist_ok=True)
    for n in names:
        shutil.move(str(roots[0] / f"{n}.wav"), stems[n])
    shutil.rmtree(work, ignore_errors=True)
    return stems


def refine_attack(y: np.ndarray, sr: int, t: float, pre: float = 0.030, post: float = 0.030,
                  frac: float = 0.2) -> float:
    """Move an onset estimate to the point where the waveform envelope starts rising.

    Spectral-flux peaks lag or lead the audible attack by up to one analysis hop
    (11.6 ms) plus window centering. The waveform envelope is sample-accurate.
    """
    a = max(0, int((t - pre) * sr))
    b = min(len(y), int((t + post) * sr))
    if b - a < 32:
        return t
    win = max(1, int(0.0015 * sr))
    env = np.convolve(np.abs(y[a:b]), np.ones(win, dtype=np.float32) / win, mode="same")
    peak = int(np.argmax(env))
    floor = float(np.min(env[:peak + 1])) if peak > 0 else 0.0
    thr = floor + frac * (float(env[peak]) - floor)
    i = peak
    while i > 0 and env[i] > thr:
        i -= 1
    return (a + i) / sr


DRUM_BANDS = ((30, 150), (150, 600), (600, 3000), (3000, 11000))


def drum_onsets(drum_wav: Path) -> tuple[list[dict], dict[int, float]]:
    """All drum-stem onsets of the whole song, multi-labelled per TŒRN lane.

    Classification uses the per-band *rise* in energy (spectral flux) at the hit,
    normalized per song, not absolute band energy: a decaying kick tail or bass
    bleed otherwise makes almost every hit look like a kick. One onset may feed
    several lanes (kick + hat on the same 16th), each with its own band strength.
    """
    librosa = ti.librosa
    y, sr = librosa.load(drum_wav, sr=22050, mono=True)
    y44, sr44 = librosa.load(drum_wav, sr=ti.WAV_SR, mono=True)
    hop = 256
    env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=hop)
    frames = librosa.onset.onset_detect(onset_envelope=env, sr=sr, hop_length=hop,
                                        backtrack=False, units="frames", delta=0.12, wait=2)
    if len(frames) == 0:
        return [], {}
    times = librosa.frames_to_time(frames, sr=sr, hop_length=hop)
    S = np.log1p(10.0 * np.abs(librosa.stft(y, n_fft=1024, hop_length=hop)))
    freqs = librosa.fft_frequencies(sr=sr, n_fft=1024)
    flux = np.pad(np.maximum(0.0, S[:, 2:] - S[:, :-2]), ((0, 0), (2, 0)))
    feat = np.array([[float(flux[(freqs >= a) & (freqs < b), fr:fr + 3].mean()) for a, b in DRUM_BANDS]
                     for fr in frames])
    feat /= np.maximum(1e-9, np.percentile(feat, 95, axis=0))
    lo, lm, mi, hi = feat.T

    kick = (lo >= 0.45) & (lo >= 1.15 * mi)
    # A snare/clap layered on a kick barely raises the mids but clearly lifts the top band.
    body = np.minimum(lm, mi) >= 0.25
    snare = ((~kick & (lm >= 0.40) & (mi >= 0.45) & (mi >= 0.6 * hi))
             | (kick & (hi >= 0.45) & body))
    hat = ((hi >= 0.40) & (lo < 0.35) & ~snare) | (kick & (hi >= 0.45) & ~body)
    perc = ~(kick | snare | hat) & (np.maximum(lm, mi) >= 0.35)
    lanes = (("kick", ti.CH_BD, kick, lo), ("snare", ti.CH_SNARE, snare, np.maximum(lm, mi)),
             ("hat", ti.CH_HH, hat, hi), ("perc", ti.CH_PERC_SOURCE, perc, np.maximum(lm, mi)))
    n_labels = kick.astype(int) + snare + hat + perc

    events = []
    best: dict[int, tuple[float, float]] = {}
    for k, t_flux in enumerate(times):
        if not n_labels[k]:
            continue
        t = refine_attack(y44, sr44, float(t_flux))
        prev_t = float(times[k - 1]) if k > 0 else t - 1.0
        next_t = float(times[k + 1]) if k + 1 < len(times) else t + 1.0
        isolation = min(1.0, (min(t - prev_t, next_t - t) + 0.03) / 0.25)
        for name, ch, mask, strength in lanes:
            if not mask[k]:
                continue
            s = float(strength[k])
            events.append({"t": round(t, 5), "s": round(s, 4), "lane": name})
            # Sample cut: prefer a strong hit where this is the only sounding drum.
            purity = 1.0 if n_labels[k] == 1 else 0.45
            score = min(s, 1.5) * purity * (0.65 + 0.7 * isolation)
            if ch not in best or score > best[ch][1]:
                best[ch] = (t, score)
    return events, {ch: t for ch, (t, _s) in best.items()}


def stem_onsets(stem: Path) -> np.ndarray:
    librosa = ti.librosa
    y, sr = librosa.load(stem, sr=22050, mono=True)
    y44, sr44 = librosa.load(stem, sr=ti.WAV_SR, mono=True)
    hop = 256
    env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=hop)
    times = librosa.onset.onset_detect(onset_envelope=env, sr=sr, hop_length=hop,
                                       backtrack=False, units="time", delta=0.07, wait=2)
    return np.asarray([refine_attack(y44, sr44, float(t)) for t in times], dtype=float)


def predict_notes(audio: Path, cache: Path):
    """Basic Pitch notes, cached as MIDI next to the analysis."""
    if not cache.exists():
        from basic_pitch.inference import predict
        _out, midi, _events = predict(str(audio))
        cache.parent.mkdir(parents=True, exist_ok=True)
        midi.write(str(cache))
    return ti.midi_notes(cache)


def merge_monophonic(notes, pitch_min: int, pitch_max: int):
    """One melodic line: merge split fragments, resolve simultaneous pitches, cut overlaps.

    Grid-independent part of toern_import.clean_monophonic_notes(); the per-step
    selection happens in the simulator after quantization.
    """
    seq = [n for n in notes if pitch_min <= int(n.pitch) <= pitch_max and (n.end - n.start) >= 0.055]
    seq.sort(key=lambda n: (float(n.start), -float(n.velocity), -float(n.end - n.start)))
    merged = []
    for n in seq:
        if not merged:
            merged.append(n)
            continue
        prev = merged[-1]
        gap = float(n.start) - float(prev.end)
        if int(n.pitch) == int(prev.pitch) and gap <= 0.07:
            prev.end = max(float(prev.end), float(n.end))
            prev.velocity = max(int(prev.velocity), int(n.velocity))
            continue
        if float(n.start) - float(prev.start) < 0.06:
            ps = float(prev.velocity) * max(0.06, float(prev.end - prev.start)) ** 0.5
            ns = float(n.velocity) * max(0.06, float(n.end - n.start)) ** 0.5
            if ns > ps:
                merged[-1] = n
            continue
        if float(n.start) < float(prev.end):
            prev.end = max(float(prev.start) + 0.03, float(n.start))
        merged.append(n)
    return merged


def snap_to_onsets(notes, onsets: np.ndarray, before: float = 0.10, after: float = 0.03):
    """Replace Basic Pitch's starts with the nearest real stem attack.

    The window is asymmetric because Basic Pitch note starts tend to lag the attack.
    """
    if len(onsets) == 0:
        return notes
    for n in notes:
        cands = onsets[(onsets >= n.start - before) & (onsets <= n.start + after)]
        if len(cands):
            n.start = float(cands[np.argmin(np.abs(cands - n.start))])
    return notes


def densest_window(beats: np.ndarray, drum_events: list[dict], bars: int = 16) -> float:
    if len(beats) == 0:
        return 0.0
    need = bars * 4
    ts = np.asarray([e["t"] for e in drum_events])
    ss = np.asarray([e["s"] for e in drum_events])
    best_i, best = 0, -1.0
    for i in range(max(1, len(beats) - need)):
        a, b = beats[i], beats[min(i + need, len(beats) - 1)]
        score = float(np.sum(ss[(ts >= a) & (ts < b)]))
        if score > best:
            best, best_i = score, i
    return float(beats[best_i])


# Default cut per lane: (stem, seconds kept, pre-roll). The simulator can re-cut from the stem.
DRUM_CUTS = {"kick": (ti.CH_BD, 0.42), "snare": (ti.CH_SNARE, 0.58), "hat": (ti.CH_HH, 0.22),
             "perc": (ti.CH_PERC_SOURCE, 0.42)}
TONAL_CUTS = {"bass": ("bass", 24, 72), "lead": ("other", 48, 96), "vocal": ("vocals", 48, 96)}


def cut_lane_samples(stems, drum_best, notes_by_lane, start_s: float, bpm: float, pack: Path) -> dict:
    """One source-derived sample per lane, plus where it was cut so it can be edited."""
    if pack.exists():
        shutil.rmtree(pack)
    pack.mkdir(parents=True)
    step_times = np.linspace(start_s, start_s + 64 * 60.0 / bpm, ti.STEPS + 1)
    out = {}
    for lane, (src, dur) in DRUM_CUTS.items():
        f = pack / f"{lane}.wav"
        if src not in drum_best:
            continue
        pre = 0.006
        ti.extract_sample(stems["drums"], f, drum_best[src], dur, pre_s=pre)
        out[lane] = {"file": f"samples/{lane}.wav", "stem": "drums", "label": lane.capitalize(),
                     "root": None, "start": round(drum_best[src] - pre, 5), "dur": dur}
    for lane, (stem, lo, hi) in TONAL_CUTS.items():
        cleaned = ti.clean_monophonic_notes(notes_by_lane[lane], start_s, bpm, lo, hi, step_times)
        roots = ti.choose_multisample_roots(cleaned, start_s, bpm, stems[stem], 1)
        if not roots:
            continue
        n = roots[0]
        dur = min(0.64, max(0.34, float(n.end - n.start) + 0.10))
        pre = 0.010
        f = pack / f"{lane}.wav"
        ti.extract_sample(stems[stem], f, float(n.start), dur, pre_s=pre)
        out[lane] = {"file": f"samples/{lane}.wav", "stem": stem, "label": f"{lane} root {int(n.pitch)}",
                     "root": int(n.pitch), "start": round(float(n.start) - pre, 5), "dur": round(dur, 4)}
    return out


def notes_json(notes):
    return [{"t": round(float(n.start), 5), "d": round(float(n.end - n.start), 4),
             "p": int(n.pitch), "v": int(n.velocity)} for n in notes]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("source")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    ti.load_dependencies()
    librosa = ti.librosa

    progress(2, "Decoding audio")
    wav = out / "source.wav"
    if not wav.exists():
        ti.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(Path(args.source).resolve()),
                "-ac", "2", "-ar", "44100", str(wav)])

    progress(5, "Separating stems (Demucs)")
    stems = run_demucs_cached(wav, out)

    progress(45, "Tracking beats")
    y, sr = librosa.load(wav, sr=22050, mono=True)
    duration = len(y) / sr
    onset_env = librosa.onset.onset_strength(y=y, sr=sr, hop_length=512)
    tempo, beat_frames = librosa.beat.beat_track(onset_envelope=onset_env, sr=sr, hop_length=512)
    bpm = float(np.asarray(tempo).reshape(-1)[0])
    if not math.isfinite(bpm) or not 40 <= bpm <= 300:
        bpm = 120.0
    beats = librosa.frames_to_time(beat_frames, sr=sr, hop_length=512)

    progress(50, "Detecting drum hits")
    drums, drum_best = drum_onsets(stems["drums"])
    auto_start = densest_window(beats, drums)

    progress(58, "Filtering melody stems")
    other_f = out / "stems" / "other_melody_filtered.wav"
    vocal_f = out / "stems" / "vocals_melody_filtered.wav"
    if not other_f.exists():
        ti.make_melody_analysis_stem(stems["other"], other_f, 180.0, 6500.0)
    if not vocal_f.exists():
        ti.make_melody_analysis_stem(stems["vocals"], vocal_f, 140.0, 5200.0)

    progress(65, "Transcribing bass")
    bass_raw = predict_notes(stems["bass"], out / "midi" / "bass.mid")
    progress(73, "Transcribing lead / keys")
    lead_raw = predict_notes(other_f, out / "midi" / "other.mid")
    progress(81, "Transcribing vocal melody")
    vocal_raw = predict_notes(vocal_f, out / "midi" / "vocals.mid")

    progress(87, "Refining note onsets")
    bass = snap_to_onsets(merge_monophonic(bass_raw, 0, 127), stem_onsets(stems["bass"]))
    lead = snap_to_onsets(merge_monophonic(lead_raw, 0, 127), stem_onsets(stems["other"]))
    vocal = snap_to_onsets(merge_monophonic(vocal_raw, 0, 127), stem_onsets(stems["vocals"]))

    lanes = {"bass": notes_json(bass), "lead": notes_json(lead), "vocal": notes_json(vocal)}

    progress(92, "Cutting samples")
    samples = cut_lane_samples(stems, drum_best, {"bass": bass, "lead": lead, "vocal": vocal},
                               auto_start, bpm, out / "samples")

    analysis = {
        "version": ANALYSIS_VERSION,
        "source": Path(args.source).name,
        "duration": round(duration, 4),
        "bpm": round(bpm, 3),
        "beats": [round(float(b), 5) for b in beats],
        "autoStart": round(auto_start, 5),
        "drums": drums,
        **lanes,
        "samples": samples,
    }
    (out / "analysis.json").write_text(json.dumps(analysis), encoding="utf-8")
    progress(100, "Done")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr, flush=True)
        raise SystemExit(1)
