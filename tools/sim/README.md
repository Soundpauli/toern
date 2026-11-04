# TŒRN simulator (standalone)

Early browser emulator of the TŒRN groovebox — concept / feel sketch, not a finished product.

## Hosted site

https://sim.tyng.app serves the Vite **build output** (`dist/`) as static files.

### Local develop

```bash
cd tools/sim
npm install
npm run dev    # http://127.0.0.1:5173
```

Sample / pattern files come from `SD-CARD-CONTENT/` (dev server mounts them at `/sd/`).

### Find MIDI songs

**Import / find MIDI** includes a BitMidi browser: search by artist or title,
sort fetched results by relevance, name or play count, and use **More results**
to fetch another page. **Load** downloads the selected arrangement directly into
the existing MIDI mapping/preview controls. Local file import remains available.
The page stays within the viewport; the two sidebars scroll independently.
Import options are collapsible, and **Send to device** follows both reset controls.
Search and downloads use BitMidi's public HTTPS endpoints with no API key or
server proxy. The sim handles unavailable service, empty searches and invalid
downloads; a new selection cancels the previous download. BitMidi files remain
subject to their respective rights; the catalog is not bundled or mirrored.

Run `node _debug/tests/test_sim_bitmidi.mjs` from the repo root for the client checks.

### MIDI sample kit — KIT 3

Open **KIT**, turn **encoder 4** to **3**, then press **encoder 1** to load.
The kit works with the MIDI importer's existing drum and melodic roles:

| Voice | WAV | Sound |
| --- | --- | --- |
| CH1 | `3/1.wav` | Tight hybrid kick |
| CH2 | `3/2.wav` | Body/snare-wire snare |
| CH3 | `3/3.wav` | Short closed hi-hat |
| CH4 | `3/4.wav` | Layered clap |
| CH5 | `3/5.wav` | Mid tom / percussion fallback |
| CH6 | `3/6.wav` | Rounded synth bass |
| CH7 | `3/7.wav` | FM electric piano / pluck |
| CH8 | `3/8.wav` | Choir/string-style pad |

All eight files are original synthesized mono PCM WAVs, 44.1 kHz / 16 bit,
with balanced peaks and faded boundaries. CH6–8 are tuned to **MIDI 60 (C4)**,
the simulator's sample root. Regenerate deterministically with
`python3 tools/sim/scripts/prepare-midi-kit.py` from the repo root (requires NumPy).
The files are included by both the dev server and production build.

This is a practical fixed kit, not a General MIDI soundfont. Cymbals share the
hat voice; miscellaneous percussion shares the tom. Sample voices are monophonic
and finite (bass 2.5 s, keys 3 s, pad 4 s at C4, shorter when pitched up).
MIDI import also uses CH11/13/14 synths and leaves the selected kit unchanged.
For arrangements with many parts, choose which tracks to keep in the mapping UI.
For example, the inspected *Man in the Mirror* karaoke reference has 13 sounding
parts at 99 BPM, including duplicated bass, guitars, electric piano and choir;
it cannot preserve every part independently in this voice layout.

### Build

```bash
npm run build   # writes dist/ (+ copies SD-CARD-CONTENT → dist/sd/)
```

### Deploy to `/var/www/sim.tyng.app`

```bash
npm run build
rsync -avz --delete \
  dist/ \
  root@tyng.app:/var/www/sim.tyng.app/
```

Apache **DocumentRoot** (same pattern as sdtool):

```apache
<VirtualHost *:80>
    ServerName sim.tyng.app
    DocumentRoot /var/www/sim.tyng.app
    <Directory /var/www/sim.tyng.app>
        Options -Indexes +FollowSymLinks
        AllowOverride None
        Require all granted
        DirectoryIndex index.html
    </Directory>
</VirtualHost>
```

Then `certbot --apache -d sim.tyng.app` for HTTPS.

## Current controls and features

All **five touch pads** are available on the panel in desktop/mobile layouts and through **Q P M R U** (touch 1–5). T4 auditions the current voice; T5 holds FN (turn encoder 1 for sample direction). In TOUCHY they play voices 1–5. During playback, T4+T5 queues a fill for the next page; requests for several voices accumulate independently, run for two pages, and finish with a velocity-127 drop. The FILL confirmation uses the requesting voice's color for one second.

Open **Faders · voices 1–8** for controls that remain active in menus and solo; their positions override restored mute snapshots. Sample filters include TIME (11/OFF), DLAY and bipolar V gain (−16 silence / 0 unity / +16 double). Pause releases voices and preserves effect tails. Synths expose all 13 INST choices and per-voice PSET slots (1–99). MIDI menus use SYC>/SYC<, PQ-I/PQ-O and a red `--` for inactive SYNC offsets. Browser audio, clock input and hardware indicators approximate the Teensy; no physical GPIO or equivalent outgoing MIDI-clock stream is generated.

Version-2 browser pattern snapshots retain notes, filters, channel volumes and BPM and still read old note-array snapshots. Presets and sample buffers remain separate. Hardware pattern transfer covers notes and tempo only.

```bash
# From the repository root:
node _debug/tests/test_sim_performance.mjs
node _debug/tests/test_sim_audio.mjs
node _debug/tests/test_menu_navigation.mjs
```

See [`docs/devdocs/docs/tools/simulator.md`](../../docs/devdocs/docs/tools/simulator.md) for the feature/approximation contract, input mapping and source ownership.
