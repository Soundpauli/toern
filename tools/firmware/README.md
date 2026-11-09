# Teensy Loader Javascript for Chromium Browsers

This folder is the TŒRN firmware flasher, **GPL-3.0** (see `LICENSE`). It is the only copy. The rest of the TŒRN repository is MIT. Live site: [firmware.toern.live](https://firmware.toern.live/).

This repository hosts a minimal web page and JavaScript library for flashing Teensy firmware via WebHID and opening a Serial connection.

## Quick Start

To test this repository go to the Github pages deployment here: https://coelacant1.github.io/Teensy-Loader-Javascript/Teensy-Loader-Example.html

[![Deploy to GitHub Pages](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/DeployPages.yml/badge.svg?branch=main)](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/DeployPages.yml)
[![pages-build-deployment](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/pages/pages-build-deployment/badge.svg?branch=gh-pages)](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/pages/pages-build-deployment)

## Repository Structure

- Teensy-Loader.js  
  The core library containing classes for FirmwareFile, TeensyFlasher, and SerialPortManager.

- Teensy-Loader-Example.html  
  A simple HTML page demonstrating how to:
  - Load a local .hex or .bin file
  - Select a Teensy device via WebHID
  - Flash the firmware
  - Open/close a serial port to display text output

- firmware/
  Example firmware files (e.g., blink_slow_Teensy40.hex, blink_slow_Teensy41.hex). These can be downloaded directly via links or used with the test page.

- .github/workflows/DeployPages.yml  
  A GitHub Actions workflow file that builds and deploys this project to the gh-pages branch, allowing GitHub Pages hosting.

## Local Development

1. Clone or download this repository.
2. If you want to run locally, open a terminal in the project’s directory and start a local server. For example:
```bash
python -m http.server
```
3. Visit http://localhost:8000/Teensy-Loader-Example.html in your browser (Chrome or Edge) and navigate to the HTML file.
4. Select your firmware file and click “Select Teensy Device” to pick a board via WebHID, then “Upload Firmware” to flash. You can also open or close a serial port to see debug output from the Teensy.

## Questions and Support

For additional information or recommendations, use the **Discussions** tab on GitHub.

## Contributing

Contributions are welcome! To contribute:
1. Fork the repository on GitHub.
2. Commit your changes with a descriptive message (git commit -m 'Add YourFeature').
3. Push the branch (git push origin main).
4. Submit a pull request on GitHub.

## License Agreement

This project is licensed under the [GPL-3.0](LICENSE). The copy in this repository is GPL-3.0, not AGPL.


## TŒRN standalone build

This fork presents the bundled v3.0 and Beta firmware presets in the shared TŒRN tool design. Its page also runs directly from a static HTTP server. For an independent production deployment:

```bash
npm ci
npm run build
```

Deploy `dist/`. Vite bundles firmware HEX files and the programming-button illustration with relative asset paths. The shared `tool-theme.css` is generated from `tools/_shared/tool-theme.css`; run `python3 tools/_shared/sync-tool-theme.py` from the repository root after changing that theme. The flashing library and GPL-3.0 attribution are retained.


## Browser setup wizard

The **Make it yours** section runs entirely in the browser. It asks about speaker,
note view, CTRL volume/page, A–E subpatterns, PMODE, FLOW, CRSR, recording,
clock/transport, analog PPQN where relevant, MIDI keys/folding, sample audition,
USB previews and screensaver timing. Brightness always uses the standard 64;
there is no brightness question. Back retains answers; session storage
keeps them while switching firmware or refreshing the page.

On the review screen, connect the running instrument at 115200 baud, then apply.
The included Beta and BETA-MODULR builds support `CONFIG` v1. Stable v3.0 remains
available unchanged and requires a Beta update before setup can be applied.
The browser constructs all 60 settings bytes, keeps the connected panel layout,
and leaves kit identity, SP0 flags and pattern/sample files alone. The instrument
validates the complete block, checks the previous CRC, waits for audio to settle,
applies the values live, and reports EEPROM verification and SD-backup status.
The browser independently reads back and compares all settings before success.

Commands are newline terminated: `CONFIG GET`, `CONFIG STOP`, and
`CONFIG SET <8-digit current CRC32> <120 hex digits>`. Replies are prefixed `CONFIG `
and contain JSON. Unsupported firmware times out without a write. The transport
works when MCP is disabled, too. There is no setup screen on the instrument.

Run `npm test` for the browser profiles and mock USB tests; run
`python3 _debug/tests/test_settings_config.py` from the repository root for the
actual firmware endpoint with mock EEPROM/audio. Build the hardware variants with
`pio run -d platformio -e teensy41` and `-e teensy41-modulr` respectively.

## Full 16 MB PSRAM diagnostic

Select **RAM test · 16 MB** in Firmware and upload it using the existing USB
flasher. After restart, open **Test all 16 MB of RAM**, connect the running Teensy,
and start the test. Its progress and PASS/FAIL result appear on the page. Restore
the usual instrument build afterward using the restore button and flasher.
EEPROM and SD files are not accessed by the diagnostic; external RAM contents
are overwritten. A Teensy 4.1 is required.

The standalone project at `ram-test/` uses the same 816 MHz CPU setting as the
instrument. It allocates no PSRAM for the program. All 4,194,304 words in the
16,777,216-byte external region are visited in every stage: six March C− stages,
then full write/read passes for zero, ones, alternating bits, address and inverse
address, and four address-dependent pseudorandom patterns (26 stages total).
Aligned 4 KB chunks are invalidated before access and flushed/invalidated after
access, so reads test physical PSRAM rather than CPU cache. These cache operations
also underpin [PJRC's official memory test](https://github.com/PaulStoffregen/teensy41_psram_memtest).

Only a completed test with exactly 16 MB detected can report PASS. Missing/8 MB
memory, mismatches, aborts, interruptions and malformed/incomplete replies cannot
pass. A mismatch includes its exact address and expected/actual word. A hard
hardware crash may prevent a final reply; the page reports an incomplete test.
Results stay in the diagnostic's internal RAM for reconnect/status readback until
power-off or reflashing. A completed pass is a diagnostic result at the current
clock and conditions, not a guarantee against every intermittent hardware fault.

Build and refresh the bundled diagnostic:

```bash
pio run -d tools/firmware/ram-test
cp tools/firmware/ram-test/.pio/build/teensy41-ramtest/firmware.hex tools/firmware/public/toern_ram_test.ino.hex
```

USB commands: `RAMTEST STATUS`, `RAMTEST START`, `RAMTEST ABORT`. JSON replies
and progress updates start with `RAMTEST `. The browser validates the diagnostic
identity, memory size, run and all 26 completed stages before showing a pass.
`npm test` includes the real C++ engine over a complete simulated 16 MB, injected
memory faults, missing memory, USB progress messages and the bundled HEX parser.


Factory-firmware / programming-button path: **Choose Teensy bootloader** is always
available. Press the physical Teensy programming button, choose the USB HID
bootloader, then upload RAM test. This path does not request a running serial
port or depend on the factory firmware. USB serial is needed only after the
RAM diagnostic boots, to start/read the test. The normal running-sketch reboot
path remains available as a convenience.


HalfKay identification: the standard bootloader USB ID is `16C0:0478`, shared
across Teensy models; it does not distinguish Teensy 4.0 from 4.1. This tool's
RAM-test image explicitly targets TŒRN/Teensy 4.1 and accepts that standard ID.
A steady red bootloader LED is normal after USB communication; blinking is not
required. References: [PJRC loader source](https://github.com/PaulStoffregen/teensy_loader_cli/blob/master/teensy_loader_cli.c)
and [PJRC bootloader LED documentation](https://www.pjrc.com/store/ic_mkl02_t4.html).


## Firmware startup defaults

The current Beta and BETA-MODULR source defaults to **LEDS 2B** (stored value 4):
two 16×16 panels with rotated panel compensation. Existing saved LED layouts
are kept; erased/invalid settings and first boot use 2B. Panel count remains 2,
independent of the orientation mode number.

At the very beginning of application `setup()`, the Teensy's built-in LED blinks
four times, 500 ms on and 500 ms off each time, followed by a 1000 ms pause.
Serial and the remaining instrument setup start after that five-second signal.
The standalone RAM diagnostic retains its separate running/pass LED behavior.

Wizard CTRL and subpattern answers override the note-view preset independently.
Transport SEND is asked when TŒRN sends MIDI clock, choosing clock+transport or
clock only; MIDI/analog followers retain their receive/arm behavior. MIDI FOLD is
asked when receiving MIDI notes. PMODE exposes OFF, ON, NEXT and SONG; SONG uses
an existing arranger sequence. Older session drafts are migrated with defaults
for new questions and their previous implied CTRL/subpattern selections kept.
Applying a setup refreshes the physical CTRL encoder, page/mute mode and idle
screensaver timer immediately, including when the device already shows DRAW.
