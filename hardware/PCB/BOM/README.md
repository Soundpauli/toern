# Rev H purchasing BOMs

Quantities are for **one unit**, excluding assembly attrition/spares. Multiply by
the production quantity and add the overage required by the assembler.
The standard build uses **one 32×16 LED panel**.

| File | Use |
|---|---|
| [Production BOM](../toern_revH/jlcpcb/production_files/BOM-toern_revH.csv) | Upload with the matching [CPL](../toern_revH/jlcpcb/production_files/CPL-toern_revH.csv) and [Gerbers](../toern_revH/jlcpcb/production_files/GERBER-toern_revH.zip) |
| [preorder-parts.csv](preorder-parts.csv) | Parts to pre-order or source for JLCPCB assembly |
| [assembly-sourcing.csv](assembly-sourcing.csv) | Complete production parts list, quantities, LCSC IDs and assembly responsibilities |
| [hand-solder-parts.csv](hand-solder-parts.csv) | Four separately purchased Teensy carrier sockets to solder at J28–J31 |
| [additional-purchased-parts.csv](additional-purchased-parts.csv) | Teensy, PSRAM, encoders, 32×16 panel, touchpads/inlay, cables, speaker and enclosure hardware |
| [Older KiCad CSV](../toern_revH/toern_revH.csv) | Reference only: differs from the production export in values, references and quantities |

## Pre-order or source for JLCPCB assembly

Arrange these parts before placing the PCB assembly order. JLCPCB solders them
onto the carrier; pre-ordering a part does not make it a hand-soldering task.

| Refs | Manufacturer part | LCSC | Qty per PCB |
|---|---|---|---|
| U3 | NXP SGTL5000XNBA3 | C5196742 | 1 |
| J3, J17 | CUI SJ1-3533 | C4991776 | 2 |
| S1 | E-Switch EG2219 | C3664594 | 1 |
| C6 | Panasonic EEE-FK1C102SV | C5686929 | 1 |
| J8 | Hirose DM3BT-DSF-PEJS | C597972 | 1 |
| U4 | Toshiba TLP2361(TPL,E(T | C107626 | 1 |

Use [JLCPCB Parts Pre-order](https://jlcpcb.com/help/article/what-is-jlcpcb-parts-pre-order-service)
or an assembler-approved sourcing route. Distributor references include Mouser
**612-EG2219** for S1, Mouser **798-DM3BT-DSF-PEJS** for J8, and RS Components
for U4. Confirm part matches and stock in the current quote before payment.
U4 is named `TLP2361V4_E` in KiCad; confirm the exact SO6 package.
The codec is **SGTL5000XNBA3**, QFN-32-1EP 5×5 mm; use the specified package.

## Already soldered by JLCPCB

The carrier assembly includes **CN1/CN2 pogo pins, 6.35 mm audio jacks,
3.5 mm MIDI jacks, encoder pin sockets, JST-PH headers, S1 and MK1**, as well as
the SMT components. Do not buy these again for hand assembly.

**CN1/CN2 are in the production BOM:** YZP0354-20100-01, **C5157282, quantity 2**.
They carry USB D−/D+ to the Teensy device pads and are assembled by JLCPCB.
Check their alignment/contact when installing the Teensy.

## Solder and wire yourself

- **Teensy carrier sockets:** buy **4 × 3M 929870-01-12-RA, 1×12 female sockets**
  from [DigiKey](https://www.digikey.de/de/products/detail/3m/929870-01-12-RA/1239316),
  then solder at J28–J31. The PCB identifier is C20926467. These are listed once
  in `hand-solder-parts.csv`.
- **Teensy:** solder two 1×24 male edge headers and two compatible 8 MB PSRAM
  chips (16 MB total), then test the memory before fitting the module.
- **JP1:** bridge one power-selection gap as described in the build guide.
- **Encoders:** fit RGB encoders/headers as needed and solder the address jumpers.
- **Panel clock/sidechain jacks:** [Amazon B09TK2HNKD](https://www.amazon.de/dp/B09TK2HNKD).
  Use three individual jacks for J25/J26/J27, with three JST-PH 2.00 mm 2-pin
  mating leads. Solder signal to tip and GND to sleeve.
- **Speaker:** solder its two leads to a **JST-PH 2.00 mm 2-pin mating cable**,
  then plug into J10. Pin 1 is ROUT− and pin 2 is ROUT+; neither goes to GND.
  The PCB-side J10 header is already assembled.

## Standard panel and touchpads

| Qty | Part | Source / assembly |
|---|---|---|
| 1 | 32×16 addressable RGB panel | [AliExpress](https://de.aliexpress.com/item/1005010071864259.html); standard display, connects to J9 |
| 3 | Touchpads | [Amazon B0BVQR1PJR](https://www.amazon.de/dp/B0BVQR1PJR); single, menu and record |
| 1 set | Touchpad inlay | [Amazon B0C744NTGN](https://www.amazon.de/dp/B0C744NTGN); fit pieces between the touchpad screws |

The smaller 16×16 build remains optional. For the standard panel, firmware LED
mode **2** means a 32×16 / 512-LED display; it does not require two separate
physical panels.

## Place the order

1. Upload the matching production BOM, CPL and Gerbers. Review component matches
   and ensure the listed JLCPCB-assembled parts, including CN1/CN2, are included.
2. Arrange the pre-order parts before releasing the assembly order. Verify any
   substitution against the specified footprint and electrical requirements.
3. Purchase the separate Teensy sockets, modules and wiring in the other CSVs.
   Module bundles may include pins, encoders or knobs; subtract those duplicates.
4. Follow the [build guide](../../../docs/BUILD.md) for soldering, wiring,
   first power-up and laser cutting.

Quantities and LCSC IDs follow the production BOM. If you change the design,
regenerate and review BOM, CPL and Gerbers together. An additional Teensy Audio
Shield is unnecessary: the SGTL5000 is already on the carrier.
Hardware licensing is described in the [project README](../../../Readme.md).
