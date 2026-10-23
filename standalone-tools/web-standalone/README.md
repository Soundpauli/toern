# TŒRN simulator (standalone)

Early browser emulator of the TŒRN groovebox — concept / feel sketch, not a finished product.

## Hosted site

https://sim.tyng.app serves the Vite **build output** (`dist/`) as static files.

### Local develop

```bash
cd standalone-tools/web-standalone
npm install
npm run dev    # http://127.0.0.1:5173
```

Sample / pattern files come from `SD-CARD-CONTENT/` (dev server mounts them at `/sd/`).

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

All **five touch pads** are available on the panel in desktop/mobile layouts and through **F1–F5**. T4 auditions the current voice; T5 holds FN (turn encoder 1 for sample direction). In TOUCHY they play voices 1–5. During playback, T4+T5 queues a fill for the next page; requests for several voices accumulate independently, run for two pages, and finish with a velocity-127 drop. The FILL confirmation uses the requesting voice's color for one second.

Open **Faders · voices 1–8** for controls that remain active in menus and solo; their positions override restored mute snapshots. Sample filters include TIME (11/OFF), DLAY and bipolar V gain (−16 silence / 0 unity / +16 double). Pause releases voices and preserves effect tails. Synths expose all 13 INST choices and per-voice PSET slots (1–99). MIDI menus use SYC>/SYC<, PQ-I/PQ-O and a red `--` for inactive SYNC offsets. Browser audio, clock input and hardware indicators approximate the Teensy; no physical GPIO or equivalent outgoing MIDI-clock stream is generated.

Version-2 browser pattern snapshots retain notes, filters, channel volumes and BPM and still read old note-array snapshots. Presets and sample buffers remain separate. Hardware pattern transfer covers notes and tempo only.

```bash
# From the repository root:
node tests/test_sim_performance.mjs
node tests/test_sim_audio.mjs
node tests/test_menu_navigation.mjs
```

See [`devdocs/docs/tools/simulator.md`](../../devdocs/docs/tools/simulator.md) for the feature/approximation contract, input mapping and source ownership.
