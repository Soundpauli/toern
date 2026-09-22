---
sidebar_position: 5
title: Fabrication
description: Gerbers, JLCPCB BOM/CPL, and how to open the KiCad project.
---

# Fabrication (rev H)

Maker steps (what to order, what is through-hole, first power) are in the repo-root [BUILD.md](../../../BUILD.md). This page is the file map.

## Open the design

1. Install a recent **KiCad** (7+; project files are `.kicad_pro` / `.kicad_sch` / `.kicad_pcb`).
2. Open [`PCB/toern_revH/toern_revH.kicad_pro`](../../../PCB/toern_revH/toern_revH.kicad_pro).
3. Project-local libraries are wired via `fp-lib-table` / `sym-lib-table` (`SJ1-3533`, `TPS22918DBVR`, `teensy`, …).

The Teensy 4.1 **symbol** is vendored at [`teensy/teensy.kicad_sym`](../../../PCB/toern_revH/teensy/teensy.kicad_sym) and `sym-lib-table` points at it with `${KIPRJMOD}`. The placed Teensy **footprint** is stored inside `toern_revH.kicad_pcb` (`myLibKicad:Teensy41`). That footprint library is not in the repo. Opening and ordering the board works from the embedded footprint. Updating the footprint from a library needs `myLibKicad` on your machine.

## Gerbers in-repo

| Export | Location |
|--------|----------|
| KiCad plot | [`PCB/toern_revH/Gerber/`](../../../PCB/toern_revH/Gerber) — Cu top/inner/bottom, mask, silk, paste, edge cuts, `.drl` |
| JLCPCB pack | [`PCB/toern_revH/jlcpcb/`](../../../PCB/toern_revH/jlcpcb) — `gerber/`, `production_files/` |

Stackup: **4 layers**, **1.6 mm**. The placed JLCPCB order Y12 (10 boards, top-side assembly) is written up in [`order-y12.html`](../../../PCB/toern_revH/jlcpcb/order-y12.html).

## JLCPCB SMT helpers

Under `jlcpcb/production_files/`:

| File | Use |
|------|-----|
| [`BOM-toern_revH.csv`](../../../PCB/toern_revH/jlcpcb/production_files/BOM-toern_revH.csv) | Comment / Designator / Footprint / LCSC / Qty |
| [`CPL-toern_revH.csv`](../../../PCB/toern_revH/jlcpcb/production_files/CPL-toern_revH.csv) | Component placement |
| `GERBER-toern_revH.zip` | Zipped gerbers for upload |

`fabrication-toolkit-options.json` stores KiCad Fabrication Toolkit plugin flags used when generating those exports.

### BOM highlights

Passives are in the CSV. Key ICs:

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

Jacks, the switch, JST headers, encoder sockets, the Teensy sockets, pogo pins, the mic, and the 1000 µF cap are in the same CSV and are through-hole. The Teensy module, Duppa encoders, LED matrix, LiPo, speaker, and case are not in the CSV. Both lists are in [BUILD.md](../../../BUILD.md).

## Ordering checklist

1. Upload `GERBER-toern_revH.zip`.
2. Confirm 4-layer, 1.6 mm, and the silk/mask colors you want.
3. If using SMT assembly: upload BOM + CPL. Resolve codec / charger substitutions before paying.
4. Solder the through-hole parts. Seat the Teensy only after the 5 V and 3.3 V rails measure right.
5. Flash firmware ([info.md](../../../info.md)). Check USB, SD, headphone audio, one encoder, and the matrix on J9.
