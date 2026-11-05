# Fabricating the board

A TŒRN ordered from [toern.live](https://toern.live/) is ready to play. Start at the [handbook](handbook/index.html). This page is only for someone building the rev H board from the design files.

The SGTL5000 codec is on that board. Flash steps live in [`info.md`](info.md).

Hardware files are [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/). Personal builds are fine. **Only Jan-Peter Kuntoff may sell TŒRN PCBs or kits** (with or without a Teensy). Teensy 4.1 is PJRC’s and is not part of that sale right. See the [README](../Readme.md).

## Order the PCB

Start with [`hardware/PCB/BOM/README.md`](../hardware/PCB/BOM/README.md).
It indexes the production BOM, the complete assembly sourcing checklist, the
hand-soldered PCB parts, and separately purchased modules/cables/case parts.
The codec is **SGTL5000XNBA3** (U3), not “STGL5000”. An LCSC number alone does not
prove a part is in JLCPCB assembly stock. Check every line in the quote, reserve
missing parts through [JLCPCB Parts Pre-order](https://jlcpcb.com/help/article/what-is-jlcpcb-parts-pre-order-service),
and wait for them to reach your private inventory before placing the assembly order.
The [pre-order list](../hardware/PCB/BOM/preorder-parts.csv) identifies parts to
secure for JLCPCB assembly: **SGTL5000XNBA3 (C5196742), SJ1-3533 (C4991776),
EG2219 (C3664594), EEE-FK1C102SV (C5686929), DM3BT-DSF-PEJS (C597972) and
TLP2361 (C107626)**. Verify availability and exact component matches in the quote.
These parts are soldered onto the PCB by JLCPCB.

The older `toern_revH.csv` differs from the production BOM (including resistor
values and quantities). Use the matching production BOM/CPL/Gerbers together;
review them against the KiCad board before manufacturing.

Upload [`hardware/PCB/toern_revH/jlcpcb/production_files/GERBER-toern_revH.zip`](../hardware/PCB/toern_revH/jlcpcb/production_files/GERBER-toern_revH.zip).

| | |
|---|---|
| Layers | 4 |
| Thickness | 1.6 mm |
| BOM | [`BOM-toern_revH.csv`](../hardware/PCB/toern_revH/jlcpcb/production_files/BOM-toern_revH.csv) |
| Placement | [`CPL-toern_revH.csv`](../hardware/PCB/toern_revH/jlcpcb/production_files/CPL-toern_revH.csv) |
| Schematic | [`schematic.pdf`](../hardware/PCB/toern_revH/schematic.pdf) |

Order the carrier assembled, including the chip parts, pogo pins, audio/MIDI jacks, encoder sockets, JST-PH headers, power switch and microphone. If the quote substitutes the codec or the charger, stop and check the footprint. Those two packages are unforgiving.

### Solder these yourself

| Refs / assembly | Work |
|---|---|
| J28–J31 | Solder **4 × 3M 929870-01-12-RA, 1×12 female sockets** — [DigiKey](https://www.digikey.de/de/products/detail/3m/929870-01-12-RA/1239316); PCB identifier C20926467 |
| Teensy 4.1 | Solder two 1×24 male edge headers and two 8 MB PSRAM chips |
| JP1 | Bridge one power-selection gap; see below |
| I2C encoders | Solder the module headers/RGB encoders if not supplied fitted, and set the address jumpers |
| Clock/sidechain cables | Solder the panel jack terminals to JST-PH 2.00 mm 2-pin mating leads |
| Speaker cable | Solder the speaker leads to a **JST-PH 2.00 mm 2-pin mating cable** for J10 |

**Already soldered by JLCPCB:** CN1/CN2 pogo pins, J1/J5–J7 audio jacks,
J3/J17 MIDI jacks, J2/J18–J24 encoder sockets, the JST-PH headers, S1 and MK1.
The pogo pins are present in the production BOM as **YZP0354-20100-01,
C5157282, quantity 2**. Check contact with the Teensy USB-device pads when
installing the module; do not add them to the hand-soldering list.

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
| 1 | Teensy 4.1 with PSRAM soldered on | Pattern data and the audio objects live in `EXTMEM`. A 4.1 with no PSRAM chip will not run this firmware. The README targets **16 MB** (two PSRAM chips). PJRC’s common add-on is one 8 MB chip; fit what you ordered and run PJRC’s PSRAM memory test before fitting the module |
| 4 | [Duppa I2C Encoder V2.1](https://www.duppa.net/shop/i2cencoder-v2-1/) plus their illuminated RGB encoder | The bare board does not include the knob. Schematic footprint name is `DuppaI2CEncoderV2.1`. With `exttouch` left `false` in `toern_hardware.h`, encoder indexes 0–3 use addresses **0x01, 0x41, 0x20, 0x61**. The Duppa sheet shows the solder jumpers. Each module has a 5-pin header on both sides; the board’s eight 1×05 sockets are those headers |
| 1 | **32×16 RGB LED panel — standard** | [AliExpress panel](https://de.aliexpress.com/item/1005010071864259.html). Connect **J9.1 → DIN, J9.2 → GND, J9.3 → switched +5 V**. Firmware LED mode **2** addresses all 512 LEDs on this one panel. A 16×16 panel is the smaller optional build |
| 5 | [Touchpads](https://www.amazon.de/dp/B0BVQR1PJR) | Single, menu, record, audition and FN; connect all five to J13 as listed below |
| 1 set | [Touchpad inlay](https://www.amazon.de/dp/B0C744NTGN) | Add/inlay pieces between the touchpad screws |
| 1 | Single-cell LiPo, JST-PH 2-pin | **J16 pin 1 = GND, pin 2 = VBAT** into the BQ24075. Reversed polarity is the expensive mistake. Leave the battery off until the rails look right |
| 1 | Small speaker, 4 Ω or 8 Ω | Optional. **Solder the speaker to a JST-PH 2.00 mm 2-pin mating lead** and plug into **J10** (pin 1 ROUT−, pin 2 ROUT+) |
| 1 | microSD | Handbook: SanDisk Ultra, UHS-I. 32 GB FAT32 or 64 GB exFAT both work |
| 1 | USB-C cable | Power and firmware |
| | Case and fixings | Choose a matching DXF under [`hardware/housing/`](../hardware/housing/). See laser cutting below. Include spacers, screws and nuts matched to the actual stack |

The standard touchpads use [B0BVQR1PJR](https://www.amazon.de/dp/B0BVQR1PJR)
with [B0C744NTGN inlay](https://www.amazon.de/dp/B0C744NTGN) added between the
screws. Fit the inlay before final tightening and connect the five pads to the
single/menu/record/audition/FN inputs shown below.

## Prepare and solder the Teensy

1. With the module unplugged, solder **two Teensy-compatible 8 MB PSRAM chips**
   to the underside for **16 MB** total. Follow [PJRC’s placement/orientation photos](https://www.pjrc.com/store/psram.html),
   align each pin-1 mark, use flux and inspect for bridges. These are PSRAM, not flash chips.
2. Run the memory test linked by PJRC and confirm **16 MB detected and passing**.
   A successful firmware link only describes allocated memory; it cannot test fitted RAM.
3. Solder **two 1×24, 2.54 mm male headers** to the Teensy’s long edges.
   Keep them perpendicular using the unpowered carrier sockets as an alignment jig;
   remove the module before soldering anything else on the carrier. The four
   1×12 carrier sockets J28–J31 receive these 48 pins.
4. Check [PJRC’s VIN/VUSB isolation instructions](https://www.pjrc.com/teensy/schematic.html)
   before powering through the carrier. Isolate VIN from VUSB if also connecting
   the Teensy’s own USB connector to a host while the carrier supplies VIN.
5. Check the JLCPCB-fitted CN1/CN2 tips contact the **USB-device D−/D+ pads**,
   not the USB-host pads. Keep enough clearance for the underside PSRAM.
6. Bridge **one** side of carrier JP1 as described above. Do not confuse this
   solder jumper with the Teensy VIN/VUSB isolation pads.

## Wire the controls and audio

The standard build needs **six JST-PH mating cables**:

| Qty | Cable | Connection | Source |
|---|---|---|---|
| 1 | JST-PH 5-pin, 2.00 mm | Five touchpads on J13 | [Amazon B0CBWZVNXS](https://www.amazon.de/dp/B0CBWZVNXS) |
| 4 | JST-PH 2-pin, 2.00 mm | Speaker J10 + clock/sidechain jacks J25/J26/J27 | [Amazon B0CBX13ZS2](https://www.amazon.de/dp/B0CBX13ZS2) |
| 1 | JST-PH 3-pin, 2.00 mm | LED panel on J9 | [Amazon B0CBWZ5KGJ](https://www.amazon.de/dp/B0CBWZ5KGJ) |

Solder the speaker and panel-jack terminals to the 2-pin cable leads.

Build cables with **JST-PH 2.00 mm mating housings and crimp contacts**. Connector
pin numbers refer to the PCB footprint, not the wire colour or a view into the plug.
Check continuity with power disconnected before inserting a cable.

Clock panel jack source: [Amazon B09TK2HNKD](https://www.amazon.de/dp/B09TK2HNKD).
Use three individual jacks for clock out, clock in and sidechain out, with
three JST-PH 2-pin mating cables. Verify tip/sleeve terminals and panel fit.

| Connection | Wiring |
|---|---|
| Matrix J9 | Pin 1 → matrix DIN, pin 2 → GND, pin 3 → +5 V. Use DIN, not DOUT. Standard build uses one 32×16 panel (512 LEDs); 16×16 is optional |
| Clock out J25 | Pin 1 → panel TS jack tip, pin 2 GND → sleeve. Buffered 5 V output, Teensy 37 |
| Clock in J26 | Pin 1 → TS tip, pin 2 GND → sleeve. Buffered 3–5 V input, Teensy 38; do not feed bipolar modular voltages directly |
| Sidechain out J27 | Pin 1 → TS tip, pin 2 GND → sleeve. Buffered 5 V trigger output, Teensy 28; this is a trigger, not an audio input |
| Speaker J10 | Pin 1 = ROUT−, pin 2 = ROUT+. Solder the speaker leads to a **JST-PH 2.00 mm 2-pin mating cable**, then plug into J10; **neither terminal goes to GND**. Enable VOL → SPKR after testing headphones |
| Capacitive touch J13 | Touch 1 = pin 1 (Teensy 2), touch 2 = pin 3 (Teensy 3), touch3 = pin 2 (Teensy 4). Pin 4 = touch4 (6), pin 5 = touch5 (39). No power or GND on this header |
| External buttons | With `exttouch = true`: A = J12.2 (Teensy 5), B = J12.1 (22), C = J12.3 (15), D = J14.1 (14). Each button closes to GND; J12.5 is GND. These are active LOW, not capacitive pads |
| Battery J16 | Pin 1 GND, pin 2 VBAT. Check pack polarity before connecting |

Fit the four **DuPPa I2C Encoder V2.1** modules, RGB encoders and their two 1×5
male headers per module. Set solder address jumpers **before mounting**:

| Encoder (left to right) | Address | Closed address jumpers |
|---|---|---|
| 1 | 0x01 | A0 |
| 2 | 0x41 | A6, A0 |
| 3 | 0x20 | A5 |
| 4 | 0x61 | A6, A5, A0 |

All other address jumpers remain open. See the [DuPPa hardware documentation](https://github.com/DuPPadotnet/I2CEncoderV2.1).
The carrier’s encoder headers expose **1 GND, 2 +5 V, 3 SDA (18),
4 SCL (19), 5 INT (27)**. Match the module labels on both headers, not just the
physical orientation; use the eight sockets J2/J18–J24. Test all four addresses
and rotary/push/RGB behaviour before closing the case.

## Laser-cut the enclosure

1. Use [`32_16.dxf`](../hardware/housing/32_16.dxf) for the standard 32×16 build,
   or [`16_16.dxf`](../hardware/housing/16_16.dxf) for the optional smaller build, with
   [`COMMON.dxf`](../hardware/housing/COMMON.dxf) for the desktop stack.
   For the 36 HP modular panel use [`modular_2parts.dxf`](../hardware/housing/modular_2parts.dxf)
   (both sheets) or the separate `modular.dxf` and `modular_front.dxf` files.
   Do not cut both the combined file and its separate copies.
2. Import in **millimetres, 1:1**, with no “fit to page”. Check a known dimension:
   modular base **182.5 × 128.5 mm**, front sheet **172 × 86 mm**.
   Keep `SCHNITT` as cut and `GRAVUR` as engrave; exclude clearance guides.
   Convert engraving text to paths if the cutter cannot use Space Grotesk Bold.
3. Set a starting **0.125 mm toolpath offset into the waste**: outside the
   outer perimeter, inside holes/cutouts. If the laser software asks for the
   **full kerf width**, enter **0.250 mm** to obtain that 0.125 mm offset.
   If it asks for the offset itself, enter 0.125 mm. Signs vary by application;
   inspect the preview rather than copying an old “−0.125” value.
   Apply compensation only once; do not offset an already compensated export.
4. Cut a small fit coupon in the actual laser-safe acrylic, measure it, then
   adjust compensation for your machine and sheet. Choose sheet thickness to
   match your housing and actual stack. The modular reference uses **2 mm base,
   8 mm LED, 2 mm front**, but this is an illustrative stack, not a verified
   thickness specification for every housing.
5. Engrave first, cut internal holes next, outer profiles last. Dry-fit jacks,
   encoders, the matrix, Teensy and USB pogo contacts before cutting a full set.
6. On the modular panel, fit the PCB **flat underside toward the panel**.
   PCB mounts use **M2 / 2.2 mm holes**; front clamps use **M2.5 / 2.8 mm holes**
   and matching base slots. Provide flush screw heads or an LED spacer gap.
   Check rail/rear component clearance and select spacer/screw lengths from the
   actual parts, following the [modular laser notes](../hardware/housing/exports/ModularPanel/LASER_README.txt).

The older [`tools/case/TOERN_M1_FINAL.dxf`](../tools/case/TOERN_M1_FINAL.dxf)
and browser case generator use an older board template. Check rev H mounting
holes before using them.

## First power

Do this before you trust a battery or a full LED matrix.

1. SMT is on the board. Look at the SGTL5000 (`U3`) and the BQ24075 (`IC1`) for bridges.
2. Solder the four Teensy sockets (J28–J31). Check the already assembled pogo pins (CN1 = D−, CN2 = D+). USB from the board jack reaches the Teensy through those contacts.
3. Leave the Teensy out. Leave the LiPo unplugged. **JP1** must already be bridged (VYS for normal use, VBUS for a USB-only check). Plug USB-C, slide **S1** on.
4. Measure before anything else. You want about **5 V** on the switched rail, **3.3 V** from U1/U7, **1.8 V** from U2. If a rail is wrong, unplug and stop.
5. Power off. Seat the Teensy. PSRAM is already soldered on the module. Power on again and confirm 5 V and 3.3 V are still sane.
6. Flash with PlatformIO. Steps and the exact board menu are in [`info.md`](info.md). USB type is **Serial + MIDI + Audio**. If the computer sees nothing, the pogos are not touching the Teensy USB pads, or the Teensy needs its PROGRAM button once.
7. Power off. Insert the microSD. Power on. Headphones on **J1**. You want USB serial, a lit matrix once J9 is wired, one encoder that turns, and headphone audio. Battery percentage (Menu → ETC → BATT) only means something after J16 is wired and the divider (R15 1 MΩ, R19 1.5 MΩ) matches the firmware constants.

## SD card

The firmware does not mount the card as a USB disk. Format with the SD Association formatter (FAT32 up to 32 GB, exFAT for 64 GB). Copy a starter layout to the **root** of the card.

The handbook points at [audioconvert.tyng.app](https://audioconvert.tyng.app/) → **SD CARD** → `SD-CARD-CONTENT.zip`. That zip is not in git. If the site is down, put 44.1 kHz mono 16-bit WAVs on the card with the CLI in [`tools/sdtool`](../tools/sdtool) or with a card reader. The card server answers from any screen. Insert and eject only with the unit off.

## Change the firmware

Build with `pio run -d platformio` from the repo root. The Arduino IDE will not apply [`platformio/scripts/platformio_patch_dependencies.py`](../platformio/scripts/platformio_patch_dependencies.py), so a menu-only edit can still ship a wrong sampler.

Where a new menu page has to land, and which EEPROM bytes are already taken: [firmware edits](devdocs/docs/contributing/firmware-edits.md).
