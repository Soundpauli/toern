# Repository layout

`***` means local-only/ignored by Git; it does not imply a live deployment. Existing tracked annotation source stays tracked.

```text
toern/
├── platformio/              Arduino source, src/, scripts/, config and flash.toern
├── hardware/
│   ├── PCB/
│   └── housing/             Fusion projects, DXFs, previews and export scripts
├── website/             *** public website only
├── docs/
│   ├── devdocs/
│   ├── handbook/
│   ├── BUILD.md
│   ├── info.md
│   └── SPELL.md
├── tools/
│   ├── firmware/            formerly teensyloader-standalone
│   ├── sdtool/              formerly sd-tool-standalone
│   ├── sim/                 formerly web-standalone
│   ├── audioconvert/        formerly audio-converter-standalone
│   ├── case/                formerly case-generator; case.toern.live planned only
│   ├── colorsheme/          existing color editor; no new subdomain configured
│   ├── toern-mcp/           standalone local remote bridge
│   └── _shared/             CSS sources and synchronization helper
├── _debug/
│   ├── tests/               host regression tests
│   ├── analysis/            audio-quality and hardware-audit scripts
│   └── audiofixes/       *** existing recordings, reports and local Python environment
└── _private/
    ├── cost_planner/     *** cost-planner/management application (one app)
    ├── invoice/          *** existing separate invoice application
    ├── cmgmt/            *** existing built customer-management assets
    ├── internal-docs/    *** existing private notes and BOM workbook
    └── helpers/
        ├── annotate/        existing tracked annotation tool
        ├── legacy-website-tools/ *** preserved former website tool copies
        ├── dev-editor.js         *** local website editor
        ├── dev-runner.js         *** local website development runner
        ├── tools-index.html      *** old placeholder
        └── update_instagram_feed.py *** website maintenance helper
```

The cost-planner was not duplicated into a new “mgmt” directory. The invoice source and cmgmt build already existed separately and were preserved as found. Website runtime Netlify functions and their audio-conversion helper remain with the website.

Run `pio run -d platformio` from the repository root, or run `platformio/flash.toern` to build and upload. The optional no-MCP build is `pio run -d platformio -e teensy41-no-mcp`.

Each browser tool builds/serves from its own directory. `tools/case` includes its own CSS/favicon and can be served with `npm start --prefix tools/case`. Its subdomain has not been configured or deployed. Shared CSS is copied with `python3 tools/_shared/sync-tool-theme.py`.

Run host checks with `python3 _debug/run_tests.py`. Hardware analysis programs remain opt-in and require an explicitly selected device. Their dependencies are in `_debug/analysis/requirements-audio-test.txt`.

Local deployments must use the new base directories: `tools/firmware`, `tools/sdtool`, `tools/sim`, `tools/audioconvert`, `docs/devdocs` and `docs/handbook`. Existing hosting/DNS settings were not changed.

The modular Fusion script is registered at `hardware/housing/exports/ModularPanel`; a rebuild and archive export were verified from that location. Other export scripts can be added from their corresponding folders under `hardware/housing/exports`.

Validation after the move: firmware (`teensy41`), firmware-loader, simulator and developer-doc production builds pass. All seven JavaScript host test scripts pass. Fifty-eight of 63 Python scripts pass when optional MCP/audio requirements are installed. Five existing harness failures were reproduced with the original paths: `test_configured_release.py`, `test_encoder_step_readback.py`, `test_pcm_activity.py` (mock PlatformIO environment lacks `PioPlatform`), `test_pause_effects.py` (outdated touch-fill/autogen stubs), and `test_pattern_boot_recovery.py` (missing storage-progress stubs).
