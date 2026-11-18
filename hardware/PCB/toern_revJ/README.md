# Toern revJ

RevJ adds the USB host connector and pogo contacts while keeping revH at its
committed version. Open `toern_revJ.kicad_pro` for the current design.

## USB host

J33 is the USB-C host receptacle. It uses the Teensy host +5 V output and the
existing USBHost_t36 firmware. R11/R33 are separate 56 kΩ CC pull-ups; there is
no additional CC controller or carrier VBUS switch.

All seven pogo contacts use YZP0354-20100-01, LCSC C5157282. Host contacts
CN3–CN7 use the same 2.2 mm copper pad, 0.8 mm drill and 3D model as CN1/CN2,
with a footprint variant that omits the circle over the solder pad.

| Contact | Signal | Teensy symbol pin | Board center X, Y (mm) |
|---|---|---|---|
| CN1 | Device D− | 66 | 116.07, 33.26 |
| CN2 | Device D+ | 67 | 116.07, 35.80 |
| CN3 | 5V-HOST | 55 | 121.91, 39.10 |
| CN4 | D-HOST | 56 | 124.45, 39.10 |
| CN5 | D+HOST | 57 | 126.99, 39.10 |
| CN6 | GND | 58 | 129.53, 39.10 |
| CN7 | GND | 59 | 132.07, 39.10 |

## Reviewed corrections

D2 retains the BAT54S correction in schematic and PCB: pin 1 = GND,
pin 2 = +3.3 V, pin 3 = common. The selected part is C171786 in SOT-23.

The accepted CN1/CN2 positions retain the November 10 change in `3a23c7d`.
They match the nominal [PJRC Teensy 4.1 dimensions](https://www.pjrc.com/teensy/dimensions.html)
and have 2.54 mm pitch. U6 remains at (144.77, 34.53) mm, rotation 0°.

The September 11 production export in `b1f796f`, unchanged through September 12,
placed CN1 at (116.27, 33.03) and CN2 at (116.27, 36.03), with 3.00 mm pitch.
The source PCB, production CPL, copper Gerber and PTH drill agree on those
positions; the actual files uploaded for the September 12 order are not verified.

[Position comparison](review/pogo-position-comparison.png),
[vector version](review/pogo-position-comparison.svg), and
[September production evidence](review/september-production-pogo-evidence.json).
The comparison includes XenGi's Teensy41 footprint and the supplied PJRC drawing.
All 48 outer U6 pad centers, sizes and drills match XenGi. The current device
contact centers are 0.22 mm left of XenGi's; the host centers are 0.0508 mm right
and 0.0008 mm below XenGi's. Current pogo centers follow PJRC's nominal drawing.

## Validation and manufacturing files

On November 18, 2026, KiCad DRC reported zero errors and zero unrouted
connections. There are 33 remaining library and silkscreen warnings. Physical
USB operation and fabricator impedance verification have not been established
by this check.

This folder contains copied revH BOM/CPL/review files alongside revJ Gerbers.
Review or regenerate a matching BOM, CPL and Gerber set before production.
The existing `tools/pcb/export_jlcpcb.py` still targets revH.
