# Fabricating the board

A TŒRN ordered from [toern.live](https://toern.live/) is ready to play. Start at the [handbook](handbook/index.html). This page is only for someone building the rev H board from the design files.

The SGTL5000 codec is on that board. Flash steps live in [`info.md`](info.md).

Hardware files are [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). Personal builds are fine. Selling boards needs written consent (see the [README](Readme.md)).

## Order the PCB

Upload [`PCB/toern_revH/jlcpcb/production_files/GERBER-toern_revH.zip`](PCB/toern_revH/jlcpcb/production_files/GERBER-toern_revH.zip).

| | |
|---|---|
| Layers | 4 |
| Thickness | 1.6 mm |
| BOM | [`BOM-toern_revH.csv`](PCB/toern_revH/jlcpcb/production_files/BOM-toern_revH.csv) |
| Placement | [`CPL-toern_revH.csv`](PCB/toern_revH/jlcpcb/production_files/CPL-toern_revH.csv) |
| Schematic | [`schematic.pdf`](PCB/toern_revH/schematic.pdf) |

Ask for SMT assembly of the chip parts (SGTL5000, BQ24075, regulators, USB-C, microSD, the small passives). If the quote substitutes the codec or the charger, stop and check the footprint. Those two packages are unforgiving.

### Solder these yourself

The BOM includes them so the positions are documented. Their footprints are through-hole, so a normal SMT quote leaves them empty:

| Refs | Part |
|------|------|
| J28–J31 | 3M 1×12 sockets, LCSC C20926467. The Teensy sits here |
| CN1, CN2 | Pogo pins, LCSC C5157282. USB D− and D+ from the USB-C jack into the Teensy |
| J1, J5–J7 | 6.35 mm jacks, LCSC C368502 |
| J3, J17 | 3.5 mm MIDI jacks, LCSC C4991776 |
| J2, J18–J24 | 1×05 pin sockets, LCSC C50950. Encoder plugs |
| J9–J16, J11.5, J25–J27 | JST-PH headers (2, 3, and 5 pin) |
| S1 | Power switch EG2219, LCSC C3664594 |
| MK1 | Electret mic, LCSC C529951 |
| C6 | 1000 µF electrolytic, LCSC C5686929 |
| JP1 | Solder jumper, not in the BOM. Open until you bridge it. See below |

Pin 1 on a JST header is the marked pin on the footprint. Match the schematic before crimping a cable.

### JP1 — pick one bridge

The jumper is three open pads. The center pad is the “on” side of the power switch. Until you solder one gap, the switch does not connect +5 V and the board stays off.

- **Normal use:** bridge the center pad to **VYS** (the end opposite the pin-1 mark). +5 V then comes from the BQ24075 output, so USB and a LiPo both run the board and the switch turns that rail on and off.
- **USB only:** bridge the center pad to **VBUS** (the pin-1 end). +5 V is raw USB 5 V. Pull the cable and the rail dies, battery or not. Use this while you are still checking the board.

Solder one gap. Bridging both shorts USB 5 V to the charger output.

## Buy separately

These are not on the JLCPCB BOM.

| Qty | Part | Notes |
|-----|------|--------|
| 1 | Teensy 4.1 with PSRAM soldered on | Pattern data and the audio objects live in `EXTMEM`. A 4.1 with no PSRAM chip will not run this firmware. The README targets **16 MB** (two PSRAM chips). PJRC’s common add-on is one 8 MB chip; fit what you ordered and check the linker’s EXTMEM size after the first build |
| 4 | [Duppa I2C Encoder V2.1](https://www.duppa.net/shop/i2cencoder-v2-1/) plus their illuminated RGB encoder | The bare board does not include the knob. Schematic footprint name is `DuppaI2CEncoderV2.1`. With `exttouch` left `false` in `toern.ino`, encoder indexes 0–3 use addresses **0x01, 0x41, 0x20, 0x61**. The Duppa sheet shows the solder jumpers. Each module has a 5-pin header on both sides; the board’s eight 1×05 sockets are those headers |
| 1 | 16×16 WS2812 / NeoPixel matrix | Data leaves **J9 pin 1** through R20 (220 Ω) from Teensy pin 17. **J9 pin 2** is GND. **J9 pin 3** is switched 5 V (TPS22918). A second matrix can be chained from the first module’s data out; firmware allows two (`LED_MODULES`) |
| 3 | Touch pads | Pins **2, 3, 4** (single, menu, record). They are on **J13** with pins 6 and 39. J13 has no power pin; take 3.3 V from a header that has it (J12, J14, or J15) and check the schematic before crimping. The record input comment in firmware names a TTP223-style module |
| 1 | Single-cell LiPo, JST-PH 2-pin | **J16 pin 1 = GND, pin 2 = VBAT** into the BQ24075. Reversed polarity is the expensive mistake. Leave the battery off until the rails look right |
| 1 | Small speaker, 4 Ω or 8 Ω | Optional. **J10** is the PAM8403 output (pin 1 ROUT−, pin 2 ROUT+) |
| 1 | microSD | Handbook: SanDisk Ultra, UHS-I. 32 GB FAT32 or 64 GB exFAT both work |
| 1 | USB-C cable | Power and firmware |
| | Case | [`tools/case-generator/TOERN_M1_FINAL.dxf`](tools/case-generator/TOERN_M1_FINAL.dxf), kerf about −0.125 mm. The case generator in that folder is a demo, and its hole template was drawn around an older board. Check holes against rev H before cutting a stack |

## First power

Do this before you trust a battery or a full LED matrix.

1. SMT is on the board. Look at the SGTL5000 (`U3`) and the BQ24075 (`IC1`) for bridges.
2. Solder the Teensy sockets (J28–J31) and the two pogo pins (CN1 = D−, CN2 = D+). USB from the board jack reaches the Teensy only through those pogos.
3. Leave the Teensy out. Leave the LiPo unplugged. **JP1** must already be bridged (VYS for normal use, VBUS for a USB-only check). Plug USB-C, slide **S1** on.
4. Measure before anything else. You want about **5 V** on the switched rail, **3.3 V** from U1/U7, **1.8 V** from U2. If a rail is wrong, unplug and stop.
5. Power off. Seat the Teensy. PSRAM is already soldered on the module. Power on again and confirm 5 V and 3.3 V are still sane.
6. Flash with PlatformIO. Steps and the exact board menu are in [`info.md`](info.md). USB type is **Serial + MIDI**. If the computer sees nothing, the pogos are not touching the Teensy USB pads, or the Teensy needs its PROGRAM button once.
7. Power off. Insert the microSD. Power on. Headphones on **J1**. You want USB serial, a lit matrix once J9 is wired, one encoder that turns, and headphone audio. Battery percentage (Menu → ETC → BATT) only means something after J16 is wired and the divider (R15 1 MΩ, R19 1.5 MΩ) matches the firmware constants.

## SD card

The firmware does not mount the card as a USB disk. Format with the SD Association formatter (FAT32 up to 32 GB, exFAT for 64 GB). Copy a starter layout to the **root** of the card.

The handbook points at [audioconvert.tyng.app](https://audioconvert.tyng.app/) → **SD CARD** → `SD-CARD-CONTENT.zip`. That zip is not in git. If the site is down, put 44.1 kHz mono 16-bit WAVs on the card with the CLI in [`standalone-tools/sd-tool-standalone`](standalone-tools/sd-tool-standalone) while Menu → ETC → SD is open, or with a card reader. Insert and eject only with the unit off.

## Change the firmware

Build with `pio run` from the repo root. The Arduino IDE will not apply [`scripts/platformio_patch_dependencies.py`](scripts/platformio_patch_dependencies.py), so a menu-only edit can still ship a wrong sampler.

Where a new menu page has to land, and which EEPROM bytes are already taken: [firmware edits](devdocs/docs/contributing/firmware-edits.md).
