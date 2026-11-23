# TŒRN revJ board explorer

Standalone Three.js explorer with the complete component catalog, 3D board, pin connections and inspector. `website/board-explorer/` contains the embedded version used immediately after **Open & Hackable.** on the homepage. The embedded version shows only the model and right sidebar, with a component selector for keyboard access.

## Run and check

No dependency installation or build is needed: Three.js 0.180.0 and the required addons are vendored locally.

```sh
npm start --prefix tools/board-explorer
npm run check --prefix tools/board-explorer
```

Open `http://127.0.0.1:3813/`. The carrier and encoder PCBs default to black; the Teensy defaults to hidden. White component labels and the Ghost board control are omitted. Four DuPPa modules and their RGB encoders are selectable. Their header stack height is estimated; shaft illumination colors are illustrative.

## Keep the website embed current

Edit the shared application here, then run:

```sh
python3 tools/board-explorer/scripts/sync-website.py
python3 tools/board-explorer/scripts/sync-website.py --check
```

The sync copies application files, models, data, licenses and the minimal Three.js runtime. It generates the embed HTML from the standalone entry point. Embedding styles and the homepage section behavior are maintained in `embed/`. The homepage integration is in `website/index.html`.

The embedded canvas uses Ctrl/Command + wheel for zoom so normal wheel input can scroll the website. On small screens the inspector stacks below the board. Rendering pauses while the canvas is outside the visible area.

## Refresh the carrier snapshot

From this directory, run `python3 extract.py` and `python3 prepare.py` after changing the revJ PCB. These read `hardware/PCB/toern_revJ/toern_revJ.kicad_pcb`; they do not edit the hardware source. Regenerate `assets/revj.glb` with KiCad's GLB export using components, pads, soldermask, silkscreen and STEP substitution, then sync the website embed. Geometry and metadata must come from the same PCB snapshot.

The module models come from `hardware/PCB/i2cencoder_v2_1/extra/upstream/Hardware/STEP/EncoderI2Cv21.step` and `RGB encoder.step`. The upstream module STEP does not include the local TC2030 ICSP change. Preserve `assets/encoder-NOTICE.txt` and `assets/encoder-LICENSE.txt` when redistributing. Three.js includes its MIT license in `vendor/three/LICENSE`.

## Netlify deployment

For the standalone tool, use a separate Netlify project with this folder as its publish directory and no build command. Do not publish this directory over the marketing site's root.

The marketing website target supplied for this task is `shiny-sherbet-23986f`. Once signed into an account with access, publish the complete existing `website/` directory to that project, including its existing functions and assets. The current CLI account could not access that project, so no production deployment was performed as part of this change.
