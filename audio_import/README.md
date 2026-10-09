# TŒRN Audio Import

## Simulator import (recommended)

```
cd standalone-tools/web-standalone
npm run dev            # http://127.0.0.1:5173
```

Menu → **Import Audio (MP3)** → drop a file. The dev server runs `toern_analyze.py`
with this folder's `.venv` and caches everything per file hash in `cache/<hash>/`
(stems, Basic Pitch MIDI, samples, `analysis.json`), so dropping the same file again
is instant.

The analyzer only extracts raw, unquantized events. All timing is decided live in
the simulator:

- **Fixed tempo** (default): BPM and phase are fitted to all drum hits of the song
  (constant-tempo least-squares on the 16th grid). **Follow beats** uses the beat
  tracker instead, for tracks with drifting tempo.
- **Start "1"**: click the overview to jump to a bar, or step by bar / beat / 1/16.
- **Grid nudge**: drag the close-up or use the slider.
- **Per-lane offset** + **Auto-align lanes**: cancels each lane's median timing
  error (Basic Pitch notes usually lag a bit, vocals vary).
- **Snap window**: drop hits that are further than ±x% of a step from the grid.
- **Original track (A/B)**: plays the source in sync with the pattern.

Settings are remembered per file; **Reset to auto** reverts them.
**Push pattern + samples to device** sends PUTPAT, `/<slot>/1..8.wav` and
`IMPORTSAVE <slot>` (device: Menu → ETC → SD).

Why fixed tempo: librosa's beat times land ~20–30 ms after the real attacks and
jitter by a frame, which pushed V19's beat-anchored grid notes into neighbouring
steps. The fitted grid keeps kicks within ~5 ms on herz.mp3 / night.mp3.

Drums are classified by the per-band *rise* in energy at each hit (multi-label, so a
kick and a snare/hat on the same 16th both land in their lanes); V20 used absolute
band energy, which made most hits look like kicks.

---

# CLI importer V20 (`toern_import.py`)

V20 keeps the V19 tracker quantization and short source-derived samples, but improves melodic transcription and adds a vocal melody lane on CH11.

## Voices

Sample voices:
- CH1 Kick
- CH2 Snare
- CH3 Bass
- CH4 Lead / Keys
- CH5 HiHat
- CH6 optional Perc / Tom / Clap with `--voices 6`

Synth:
- CH11 Vocal melody, monophonic transcription from the vocal stem

## Cleaner melody extraction

Raw Demucs stems are still saved next to the source in `stems/`.
For transcription only, V20 creates additional filtered analysis stems:

- `stems/other_melody_filtered.wav`: harmonic component, 180 Hz high-pass, 6.5 kHz low-pass
- `stems/vocals_melody_filtered.wav`: harmonic component, 140 Hz high-pass, 5.2 kHz low-pass

This removes much of the bass/percussion leakage before Basic Pitch sees the melody. Raw `other.wav` is still used when extracting the CH4 source sample, so filtering does not thin the actual playback sample.

## CH11 vocal melody

Vocals are not used as a sample. Basic Pitch extracts a monophonic melodic line from the filtered vocal stem and writes the detected MIDI pitches to CH11. TŒRN's CH11 sequencer path consumes stored MIDI pitch information for the poly synth.

## Quantization

Unchanged from V19: TŒRN is treated as a hard 16th-note tracker. Quarter-note beats are anchors and each interval is subdivided into four cells. Swing/microtiming is removed before writing the pattern.

## Persistent files next to source

For `song.mp3`:

```
song.mp3
stems/
  drums.wav
  bass.wav
  other.wav
  vocals.wav
  other_melody_filtered.wav
  vocals_melody_filtered.wav
  stem_names.txt
29.txt              # after successful device save/readback
```

`stem_names.txt` also includes:

```
11_vocals_Vocal melody -> CH11 poly synth
```

## Run

```
python3 toern_import.py song.mp3
```

Optional sixth sample voice:

```
python3 toern_import.py song.mp3 --voices 6
```

Upload-only still reuses the last generated local pattern/samplepack:

```
python3 toern_import.py song.mp3 --upload
```
