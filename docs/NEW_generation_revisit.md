# Revisit NEW Generation (All Genres)

This document describes how to make the **NEW** (genre track generation) output more **musically convincing**, more “produced”, and more consistent across genres. It focuses on:

- **Melody / pitch movement** (not just rhythm)
- **Harmony** (scale + chord function + voice leading)
- **Probability + conditions** for variation without chaos
- **Genre-appropriate arrangement** (intro/verse/chorus/break/outro)
- **BPM selection/adaptation** per genre (including swing/feel)
- Practical implementation guidance for this codebase (Teensy/Arduino constraints)

---

## Goals (What “musically better” means)

- **Clear tonal center**: A scale and key, with occasional tasteful borrowed tones.
- **Functional harmony**: Progression has direction (tension → release), not random chords.
- **Melodic intent**: At least one voice behaves like a lead with phrase structure.
- **Groove consistency**: Drums and bass lock; micro-timing/swing is coherent per genre.
- **Arrangement**: Sections evolve (density, register, filter/FX, pattern variation).
- **Controlled variation**: Probability/conditions create life, but keep motifs recognizable.
- **Mix/production awareness**: Register separation, velocity curves, fills, transitions.

---

## Constraints & assumptions in this project

- Patterns are grid-based; channels/voices already exist (1–8) plus extra voices (13/14) in places.
- Generation must be **fast** (avoid heavy search) and **deterministic enough** to feel intentional.
- Memory is limited; prefer small tables and simple transforms.
- Existing helpers mention: `generateGenreTrack()`, `generateSong()`, harmonic analysis utilities, probability/condition fields on notes.

The design below assumes we can:

- Choose a **key**, **scale**, and **progression**.
- Generate **bass**, **chords/pads**, **lead**, **drums**, and **ear-candy** with separate logic.
- Use **probability** and **conditions** to create variation across pages/sections.

---

## Architecture proposal (high-level)

### 1) Make generation a pipeline with shared musical state

Create a single “context” object computed once per NEW run:

- **Genre**: selected by user
- **Seed**: for repeatability (optional)
- **BPM**: chosen/adjusted by genre, plus swing/feel
- **Key**: root note + octave policy
- **Scale**: e.g., major, natural minor, dorian, phrygian, harmonic minor, pentatonic
- **Chord progression template**: per genre (4–8 bars), with functions (I, vi, IV, V…)
- **Section plan**: intro/partA/partB/break/outro (page ranges)
- **Motifs**: short melodic/rhythmic motifs reused with variation
- **Drum kit mapping**: which channel is kick/snare/hats/percs

This avoids each channel “guessing” in isolation.

### 2) Treat channels as roles, not random voices

Assign roles by channel:

- **Kick**
- **Snare/Clap**
- **Hats**
- **Percs**
- **Bass**
- **Chords/Pad**
- **Lead**
- **FX / Riser / Fill**

Then per genre, decide which roles are active and how dense they are in each section.

### 3) Use a small harmony engine

Core: pick chord roots from progression, then build chord tones from scale degrees.

Recommended:

- Build triads or seventh chords as needed.
- Apply **voice-leading**: when moving to next chord, move each chord voice to the nearest chord tone (minimize leaps).
- Constrain chord register (e.g., pads around C3–C5, bass around C1–C2, lead around C4–C6).

### 4) Generate melody with phrase + contour

Don’t random-walk notes. Use:

- **Phrase length**: 1–2 bars; repeat with variation.
- **Contour**: rise then fall, or arch, or stepwise with occasional leap.
- **Target tones**: strong beats land on chord tones (1/3/5/7).
- **Passing tones**: weak beats may use scale neighbors.
- **Motif transforms**: transpose to chord root, invert around axis, rhythm shift, ornament.

### 5) Variation via probability + conditions (structured)

Use probability/conditions for:

- **Ghost notes** (low velocity, low probability)
- **Fills** (high probability only near section boundaries)
- **Call/response** (lead responds every 2nd bar)
- **Occasional syncopation** (small probability on off-beats)

Prefer *conditional rules*:

- “Only on every 4th bar”
- “Only in chorus pages”
- “Only if previous note was present” (avoid sparse randomness)

---

## Concrete algorithms (lightweight, high-impact)

### A) BPM selection per genre (+ feel)

Pick BPM from a genre range, then optionally “snap” to musically typical values.

Examples:

- **House/Techno**: 124–132, straight, mild swing optional on hats
- **DnB**: 170–176 (or half-time 85–88), tight hats, snare on 2/4 (half-time)
- **HipHop/Trap**: 130–155 (or half-time feel 65–78), heavy swing possible
- **Ambient/LoFi**: 70–95, loose timing, fewer transients

Implementation:

- `setGenreBPM()` should set:
  - `bpm`
  - `swingAmount` (0–something small)
  - optionally a `feel` enum (STRAIGHT, SWUNG, SHUFFLE, HALF_TIME)

### B) Progression templates

Keep small lists per mode:

- Major pop: I–V–vi–IV
- Minor: i–VI–III–VII (or i–iv–VII–III)
- Dorian: i–IV–i–VII
- Phrygian: i–II–VII–i

For electronic genres, two-chord vamps are fine:

- i ↔ VI
- i ↔ VII
- I ↔ vi

Implementation:

- Define a `ProgressionTemplate { degrees[], lengthBars }`
- Choose one based on genre and maybe “mood”.

### C) Bassline generation (locks with kick, outlines chord roots)

Rules:

- Strong beats: chord root (or fifth).
- Off-beats: octave, passing tone, or approach note.
- Keep bass mostly stepwise; allow octave jumps at section transitions.

Common patterns:

- 4-on-the-floor: root on every beat; add offbeat octave occasionally.
- Half-time: long notes with syncopated pickups.

### D) Chords/pads (voice-leading + rhythm)

Pad rhythm should be sparse and section-dependent:

- Intro: 1 chord per bar
- Verse: 2 per bar
- Chorus: rhythmic stabs (house) or sustained (ambient)

Voice-leading algorithm:

1. Start chord tones in a chosen register.
2. For each next chord, for each voice choose the closest chord tone.
3. Keep spacing: avoid voices crossing; keep at least 2–3 semitones between adjacent voices.

### E) Lead melody (motif-based)

Motif generation:

- Create a base motif as scale degrees (e.g., [1, 2, 3, 5, 3, 2]).
- Map to chord context: on strong beats pick chord tones.
- Rhythm pattern: choose from a small set (e.g., 8ths with rests, triplets, syncopated).

Variation:

- Repeat motif each 2 bars
- On repeat: shift by +2 scale degrees, or change last two notes, or add grace note

### F) Drums (genre-specific groove, fills)

Keep drum generation deterministic and genre-based:

- Kick pattern templates (several)
- Snare/clap placement templates
- Hat patterns with density and occasional rolls
- Perc layer with probability and section gating

Fills:

- Only at end of a 4-bar block (e.g., bar 4)
- Use higher probability then
- Keep fill short (1 beat or 2 beats)

---

## Probability & conditions: best practices

### Use probability for “humanization”, not structure

Good:

- Ghost hats: 20–40%
- Extra percussion: 10–30%
- Small melodic ornaments: 10–25%

Avoid:

- Using probability to decide core kick/snare (destroys groove)

### Velocity: use it sparingly (but intentionally)

Velocity is the fastest way to make patterns feel “played” instead of “printed”, but it gets ugly quickly if it’s too random or too wide.

Principles:

- **Keep ranges tight** per role. Think “small performance nuance”, not “random loud/quiet”.
- **Tie velocity to musical intent** (accent pattern, section energy, ghost notes), not pure RNG.
- **Don’t modulate everything at once**. If hats are humanized, keep the kick steady.
- **Use deterministic accents first**, then add a tiny jitter (±1–6) if needed.

Safe per-role guidelines (adjust to your sample set):

- **Kick**: mostly constant (e.g., 110–125). Only small boosts at phrase starts (+3–8).
- **Snare/Clap**: strong backbeat (e.g., 105–120). Ghosts much lower (e.g., 40–65) with low probability.
- **Closed hats**: medium (e.g., 55–85) with *very small* jitter. Accents on offbeats (+5–10).
- **Open hats**: higher than closed hats (e.g., 75–105) but *rare* (conditioned to chorus/boundaries).
- **Percs**: medium-low (e.g., 45–80), varied by pattern role; avoid huge swings.
- **Bass**: tight range (e.g., 85–105). Accent chord changes; avoid random “spikes”.
- **Chords/pads**: stable (e.g., 70–95). Section-based scaling is better than per-hit jitter.
- **Lead**: expressive but bounded (e.g., 75–110). Accents on phrase peaks/targets, not every note.

Practical implementation rules:

- **Velocity curve per section**: scale overall energy by section (intro < verse < chorus), e.g. multiply by 0.85/1.0/1.1.
- **Accent masks**: predefine per-genre accent steps (e.g., 1&3, or offbeats) and apply +Δvel there.
- **Ghost-note helper**: `velGhost = velMain * 0.45` and probability-gate it; never ghost the main backbeat.
- **Clamp** all velocities to a sane min/max (avoid silent notes or clipping).

### Use conditions for section logic

Recommended condition types:

- **bar index** (mod 4 / mod 8)
- **section** (intro/verse/chorus/break/outro)
- **page range** (base pages vs generated extension pages)

Example mapping:

- Chorus adds open hat on every 2nd bar.
- Break removes kick and bass, keeps pad and FX.

---

## Genre mapping (what to do for each existing NEW genre)

The exact `genreType` values depend on current code; map these ideas to the existing list in `drawGenreSelection()` / `generateGenreTrack()`.

For each genre, define:

- **BPM range**
- **Scale/mode**
- **Progression template set**
- **Section plan**
- **Groove templates**
- **Lead behavior** (busy vs sparse, range)
- **Production cues** (density, register, transitions)

### 0) BLNK (blank)

- **Goal**: create an empty but “ready” template (kick optional off, neutral key).
- **No generation** or generate only minimal metronome-like structure if you want.

### 1) Genre A (replace with actual name)

- **BPM**: e.g. 124–128
- **Scale**: minor or dorian
- **Progression**: i–VI–VII–i or i–iv–VII–III
- **Drums**: 4-on-floor kick; clap on 2/4; hats 8ths; open hat offbeat in chorus
- **Bass**: root + octave; occasional approach note
- **Lead**: short motif, 1–2 bars, repeats with small variation

### 2) Genre B

- **BPM**: e.g. 85 (half-time 170)
- **Scale**: minor / phrygian for darker vibe
- **Drums**: snare on 3 (half-time), busy hats with rolls (conditional)
- **Bass**: long notes + syncopated pickups
- **Lead**: sparse, call/response, more slides/ornaments (if supported)

### 3) Genre C

- **BPM**: 140–150
- **Scale**: harmonic minor option for tension
- **Progression**: two-chord vamp
- **Lead**: higher register, more rhythmic syncopation
- **FX**: riser into chorus (only on boundaries)

### 4) Genre D

- **BPM**: 70–95
- **Scale**: major pentatonic / dorian
- **Pad**: sustained chords, slow voice-leading
- **Lead**: very sparse, long notes
- **Drums**: minimal, soft ghost percussion

### 5) Genre E

- **BPM**: 128–135
- **Scale**: major or mixolydian
- **Drums**: driving hats, consistent groove
- **Bass**: more active; occasional passing tones

> Implementation note: once you confirm the actual genre names/order, update this section with the real mapping. The code can use a `GenreSpec` table keyed by `genreType`.

---

## Implementation plan (how to achieve this in code)

### Step 1: Introduce `GenreSpec` + `GenerationContext`

Add small structs (in the file where generation lives):

- `GenreSpec`: bpm range, scale, progression options, groove templates, density curves
- `GenerationContext`: chosen spec + random seed + computed chord progression + section plan

### Step 2: Centralize note placement helpers

Create helpers that all generators use:

- `placeNote(channel, step, pitch, vel, len, prob, condition)`
- `placeChord(channel, step, chordTones[], voicingPolicy, prob/condition)`
- `placeDrumHit(channel, step, vel, prob, condition)`

This ensures consistent probability + conditions usage.

### Step 3: Build harmony first, then arrange voices

Order:

1. Determine chord per bar/half-bar
2. Generate bass aligned to chords + kick
3. Generate chords/pad voicing
4. Generate lead motif using chord targets
5. Add percs/FX + fills at boundaries

### Step 4: Add section-aware variation

Implement section gates:

- Intro: fewer channels, lower density
- Chorus: more hats, stronger bass rhythm, more lead notes
- Break: remove kick/bass, keep pad/FX

### Step 5: BPM adaptation & “produced” touches

After generation:

- Ensure kick and bass don’t clash (avoid bass notes exactly on kick if too dense)
- Avoid register collisions (lead above chords; bass below)
- Add 1–2 transition fills (drum fill or riser) at section boundaries

---

## Testing / evaluation checklist

- **Groove**: kick/snare/hats feel consistent for 8 bars
- **Harmony**: chords don’t sound random; cadences occur at boundaries
- **Melody**: motifs repeat and evolve; strong beats hit chord tones
- **Variation**: repeated pages aren’t identical, but still recognizable
- **Arrangement**: clear build → peak → break → return

---

## Where to hook this in the current project

Search targets:

- `generateGenreTrack()` (NEW mode generation)
- `generateSong()` (AUTO generation)
- Any harmonic analysis helpers and progression builders in `toern_helpers.ino` / related files

Recommended refactor:

- Keep existing entrypoints (`generateGenreTrack`, `generateSong`) but have them create a `GenerationContext` and call role generators.


