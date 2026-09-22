# Compile & upload

Hardware order and first power-up: [BUILD.md](BUILD.md).

Firmware for **Teensy 4.1**. Open the sketch from the **repo root** (`toern.ino` plus the `toern_*.ino` tabs). Custom audio lives in `src/` and is compiled with the sketch.

Match these board options in both Arduino IDE and PlatformIO:

| Setting | Value |
|---|---|
| Board | Teensy 4.1 |
| USB Type | **Serial + MIDI** (`USB_MIDI_SERIAL`) |
| CPU Speed | **600 MHz** |
| Optimize | **Faster** (`-O2`, `TEENSY_OPT_FASTER`) |
| USB Audio | off (Serial is required for the SD tool and diagnostics) |

Serial monitor: **115200** baud.

ITCM (RAM1 code) must stay **under 262144 bytes**. Crossing that line steals a FlexRAM bank from DTCM and the link fails (`free for local variables: -544`). Keep ISR-heavy audio (`src/effect_freeverb_dmabuf.cpp`) small.

---

## PlatformIO (preferred)

Pinned env: `platformio.ini` → `env:teensy41` (`platformio/teensy@6.0.0`). A pre-build script (`scripts/platformio_patch_dependencies.py`) applies the project’s sampler / amplitude-ramp / resampler patches to the downloaded libraries, so you do **not** need hand-edited Arduino libraries.

```bash
cd /path/to/toern
pio run                         # .pio/build/teensy41/firmware.hex
pio run --target upload         # build + Teensy Loader
pio device monitor              # 115200
pio run --target compiledb      # compile_commands.json
```

### CLI upload (no PIO upload target)

Build first, then push the hex with PJRC’s helper (same binary PlatformIO uses). If another Teensy Loader window is already open, quit it so it does not steal the board.

**macOS / Linux** (PlatformIO package install):

```bash
pio run

TEENSY_TOOLS="$HOME/.platformio/packages/tool-teensy"
"$TEENSY_TOOLS/teensy_post_compile" \
  -v \
  -file=firmware \
  -path="$(pwd)/.pio/build/teensy41" \
  -tools="$TEENSY_TOOLS" \
  -board=TEENSY41 \
  -reboot
```

**Windows** (PowerShell):

```powershell
pio run
$tools = "$env:USERPROFILE\.platformio\packages\tool-teensy"
& "$tools\teensy_post_compile.exe" -v -file=firmware `
  -path="$PWD\.pio\build\teensy41" -tools=$tools -board=TEENSY41 -reboot
```

If the helper prints **No Teensy boards were found**, tap the **PROGRAM** button on the Teensy and run the same command again.

Arduino IDE Teensyduino tools (if PlatformIO is not installed):

```bash
# typical Arduino Boards Manager path — adjust the Teensyduino version
TEENSY_TOOLS="$HOME/Library/Arduino15/packages/teensy/tools/teensy-tools/1.59.0"   # macOS
# TEENSY_TOOLS="$HOME/.arduino15/packages/teensy/tools/teensy-tools/1.59.0"        # Linux
# HEX is the .hex Arduino produced, e.g. /tmp/arduino/sketches/.../toern.ino.hex

"$TEENSY_TOOLS/teensy_post_compile" \
  -v -file=toern.ino -path=/path/to/hex-directory \
  -tools="$TEENSY_TOOLS" -board=TEENSY41 -reboot
```

`-file` is the hex **basename without `.hex`**. `-path` is the directory that contains it.

---

## Arduino IDE

1. Install [Arduino IDE](https://www.arduino.cc/) and [Teensyduino](https://www.pjrc.com/teensy/td_download.html) (1.59.x / Teensy AVR core **1.59.0** matches this tree).
2. **File → Open** `toern.ino` from the repo root (not a copy of a single tab).
3. **Tools**:
   - Board: **Teensy 4.1**
   - USB Type: **Serial + MIDI**
   - CPU Speed: **600 MHz**
   - Optimize: **Faster**
4. **Sketch → Upload** (opens Teensy Loader). Or compile only, then use `teensy_post_compile` as above.

### Libraries

Teensyduino already provides Audio, SD, SdFat, EEPROM, SPI, Wire, LittleFS, SerialFlash, WS2812Serial.

Install (Library Manager or git), versions as in `platformio.ini` where pinned:

| Library | Notes |
|---|---|
| FastLED **3.9.10** | sketch sets `FASTLED_ALLOW_INTERRUPTS 0` |
| MIDI Library **5.0.2** (FortySevenEffects) | |
| Adafruit BusIO **1.17.4** | |
| Adafruit seesaw **1.7.9** | |
| Mapf **1.0.2** | |
| [ArduinoDuPPaLib](https://github.com/Fattoresaimon/ArduinoDuPPaLib) **v1.2.0** | `i2cEncoderLibV2.h` |
| [FastTouch](https://github.com/AdrianFreed/FastTouch) | |
| [teensy-polyphony](https://github.com/newdigate/teensy-polyphony) `#548180ac` | last commit before the multi-channel MIDI API break |
| [teensy-variable-playback](https://github.com/newdigate/teensy-variable-playback) **1.0.16** | |

Arduino IDE does **not** run `scripts/platformio_patch_dependencies.py`. Either build with PlatformIO, or apply the same sampler / `ResamplingReader.h` / `playresmp.h` changes by hand (the repo copies used by PIO are `src/resamplerReader.h` and the patch script).

### Arduino CLI

```bash
arduino-cli compile --fqbn teensy:avr:teensy41:usb=serialmidi,speed=600,opt=o2std

# Upload uses Teensy Loader, not a normal serial port:
arduino-cli upload --fqbn teensy:avr:teensy41:usb=serialmidi,speed=600,opt=o2std
```

If upload cannot see the board, press **PROGRAM** on the Teensy.

---

## After flashing

USB enumerates as **Serial + MIDI**. Keep **Menu → ETC → SD** open if you use the SD serial tool. Insert or eject the Micro SD only with the unit **off**.
