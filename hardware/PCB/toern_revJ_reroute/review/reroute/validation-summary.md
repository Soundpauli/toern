# Toern revJ reroute status

The active board, project and schematic match the recoverable checkpoint byte for byte. Backplane's Board viewer is open on that board. The separate candidate is a partial reroute and requires the Backplane KiCad fork. The fork saved board format 20261031, which stock KiCad 9.0.5 rejected with “Failed to load board.” Git HEAD remained ab3ea41 throughout the checks.

## Implemented and checked

- Geometry changed on 22 nets, including the U3 +1V8 and VAG feeds, codec inputs and local I²S links, charger programming traces, both USB trunk sections and part of 5V-HOST. The [geometry comparison](candidate-geometry-and-reference.txt) found matching placements, footprint identities, primary pad geometry and net assignments for all 138 footprints and 523 pads. Outline records match. Both schematics are byte-identical.
- After zone refill, the candidate had zero unrouted connections and no reported copper, hole, width or short errors. The independent connectivity query reached every pad in all 109 multi-pad PCB nets (458 pad records). All 459 schematic pin-to-net assignments checked matched using the seven approved U6-to-pogo aliases and excluding symbols marked Exclude from board. [Candidate DRC](candidate-drc.json) · [Physical connectivity](candidate-physical-connectivity.json) · [Schematic comparison and path measurements](candidate-connectivity-and-paths.txt)
- On F.Cu, U3 VDDD to C2 is 2.541 mm at 0.25 mm. U3 VAG to C13 is 1.805 mm at 0.20 mm, down from 1.918 mm. U3 VDDIO to C11 remains 6.213 mm at 0.20/0.50 mm. IC1 IN to C32 remains 1.671 mm at 0.50 mm. U8 CT to C34 is 1.602 mm at 0.20 mm. This trial did not establish all cap-to-ground loop lengths or inductances. [Path measurements](candidate-connectivity-and-paths.txt)
- The successful host power change widened 18.60 mm of B.Cu from 0.60 mm to 0.80 mm, necking down near J19/J20. CN3 to J33 A4/B9 still traverses approximately 120.32 mm of planar copper and five transitions. J33 A9/B4 uses approximately 125.80 mm and seven transitions. The distinct R11/R33 56 kΩ CC1/CC2 pulls remain connected. [Path measurements](candidate-connectivity-and-paths.txt)
- Front copper renders were visually inspected around the codec, amp and charger. [Before front](before-front.png) · [Candidate front](candidate-front.png) · [Before back](before-back.svg) · [Candidate back](candidate-back.svg) · [Candidate In1](candidate-inner1.svg) · [Candidate In2](candidate-inner2.svg)

## USB and remaining work

0.13 mm trace width and 0.13 mm edge gap were implemented on approximately 49.75 mm of the device pair on B.Cu and 25.40 mm of the host pair on F.Cu. The device section had In2.Cu ground beneath every centerline sample at 0.10 mm intervals. The host corridor passes through the fixed J2 and J18 connector rows between pins 3 and 4. Their antipads interrupt the local In1.Cu reference at X near 45 and 66 mm. The existing 0.30 mm zone clearance and 0.25 mm minimum fill leave insufficient reference copper directly beneath both traces there. [Geometry and reference audit](candidate-geometry-and-reference.txt)

| Path | D+ planar | D− planar | D+/D− via transitions | Planar skew | Skew with nominal via lengths |
|---|---:|---:|---:|---:|---:|
| Device A | 100.901 mm | 96.171 mm | 1/2 | 4.730 mm | 3.130 mm |
| Device B | 98.009 mm | 97.213 mm | 1/2 | 0.796 mm | 0.804 mm |
| Host A | 120.227 mm | 121.126 mm | 1/3 | 0.899 mm | 4.099 mm |
| Host B | 121.323 mm | 120.126 mm | 3/1 | 1.197 mm | 4.397 mm |

These measurements run from connector pad centers to pogo centers. The last column assumes 1.60 mm per full-board layer transition for a geometric comparison. Electrical delay, connector and pogo parasitics, fabrication impedance and physical USB/audio performance were not measured. Both pairs still need fanout and return-path work to meet the targets in the [routing constraints](routing-constraints.md).

The wider automated power pass generated 30 missing connections, a short at JP1, hole-clearance violations and an In1.Cu segment despite the F.Cu/B.Cu restriction. The affected power topology was recovered before the targeted host width change. The analog pass failed to complete MICBIAS and generated a line-output clearance violation. The affected nets were restored. Several edit calls timed out after disk saves. The final file was read back and checked independently. The last plane-clearance trial failed when Backplane's KiCad IPC stopped answering Ping. The candidate hash confirmed the saved board was unchanged after that failure. [Failed power-pass DRC](candidate-power-pass-drc.json)

The final candidate has four one-spoke thermal warnings on the stacked J4 GND pads, absent from the stock KiCad 9 baseline report. The same-runtime baseline control could not be retrieved after IPC stopped responding. Candidate DRC also reports two existing courtyard overlaps whose exclusions the fork dropped from the copied project, 138 footprint-library availability warnings in the isolated project, 10 silkscreen findings and 20 existing Teensy parity findings. The active project retains its original rule settings and exclusions. [Stock KiCad baseline DRC](baseline-drc.json) · [Final candidate DRC](candidate-drc.json)

The full reroute and stock KiCad 9 acceptance criteria remain unmet. The active baseline was retained.
