---
sidebar_position: 5
title: Fabrication
description: Gerbers, JLCPCB BOM/CPL, and how to open the KiCad project.
---

# Fabrication (rev G)

## Open the design

1. Install a recent **KiCad** (7+; project files are `.kicad_pro` / `.kicad_sch` / `.kicad_pcb`).  
2. Open [`PCB/toern_revG/toern_revG.kicad_pro`](https://github.com/Soundpauli/toern/blob/main/PCB/toern_revG/toern_revG.kicad_pro).  
3. Project-local libraries are wired via `fp-lib-table` / `sym-lib-table` (`SJ1-3533`, `TPS22918DBVR`, …).

:::warning Absolute path in sym-lib-table
`sym-lib-table` currently references a **machine-specific** Teensy symbol path (`D:/PCBs/myKicadLib/...`). On another computer, retarget the `teensy` symbol library or KiCad will show missing symbols for `U6`. Footprints for the Teensy may still resolve from the PCB cache / footprint libs.
:::

## Gerbers in-repo

| Export | Location |
|--------|----------|
| KiCad plot | [`PCB/toern_revG/Gerber/`](https://github.com/Soundpauli/toern/tree/main/PCB/toern_revG/Gerber) — Cu top/inner/bottom, mask, silk, paste, edge cuts, `.drl` |
| JLCPCB pack | [`PCB/toern_revG/jlcpcb/`](https://github.com/Soundpauli/toern/tree/main/PCB/toern_revG/jlcpcb) — `gerber/`, `production_files/` |

Stackup reminder: **4 layers**, **1.6 mm**.

## JLCPCB SMT helpers

Under `jlcpcb/production_files/`:

| File | Use |
|------|-----|
| [`BOM-toern_revG.csv`](https://github.com/Soundpauli/toern/blob/main/PCB/toern_revG/jlcpcb/production_files/BOM-toern_revG.csv) | Comment / Designator / Footprint / LCSC / Qty |
| [`CPL-toern_revG.csv`](https://github.com/Soundpauli/toern/blob/main/PCB/toern_revG/jlcpcb/production_files/CPL-toern_revG.csv) | Component placement |
| `GERBER-toern_revG.zip` | Zipped gerbers for upload |

`fabrication-toolkit-options.json` stores KiCad Fabrication Toolkit plugin flags used when generating those exports.

### BOM highlights (assembled on PCB)

Not a full shopping list — see the CSV for passives. Key ICs / connectors:

| Comment | Designators | LCSC (as exported) |
|---------|-------------|--------------------|
| BQ24075TRGTR | IC1 | C544783 |
| SGTL5000XNBA3 | U3 | C5196742 |
| SPX3819 3.3V | U1, U7 | C9055 |
| SPX3819 1.8V | U2 | C24639 |
| PAM8403D | U13 | C17337 |
| TLP2361 | U4 | C107626 |
| 74LVC2G14 | U5 | C12401 |
| USB-C | J4 | C165948 |
| microSD | J8 | C597972 |
| 6.35 mm jacks | J1, J5–J7 | C368502 |
| 3.5 mm jacks | J3, J17 | C4991776 |
| EG2219 switch | S1 | C3664594 |
| JST-PH headers | J9–J16, J11.5, … | various |
| Encoder pin sockets | J2, J18–J24 | C50950 |

**Teensy 4.1**, **encoder modules**, **LED matrices**, **LiPo**, and **case** are typically **not** in the JLCPCB PCB+SMT BOM — buy/assemble separately.

## Ordering checklist

1. Upload gerbers (JLCPCB zip or re-plot from KiCad).  
2. Confirm 4-layer, 1.6 mm, and silk/mask colors you want.  
3. If using SMT assembly: upload BOM + CPL; resolve any LCSC substitutions carefully (codec / charger footprints are unforgiving).  
4. Hand-stuff / socket: Teensy, encoders, matrices, battery, optional speaker.  
5. Flash firmware; verify USB, SD, audio loopback, encoder INT, battery ADC.

## Related revisions

- [`PCB/toern_revF/`](https://github.com/Soundpauli/toern/tree/main/PCB/toern_revF) — previous board revision  
- Prefer **rev G** for new builds unless you are maintaining an older unit
