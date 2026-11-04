TOERN modular - 36 HP

modular.dxf: base panel 182.5 x 128.5 mm.
modular_front.dxf: front clamping sheet 172 x 86 mm.
Millimetres, 1:1. SCHNITT = cut; GRAVUR = engrave.
Functional labels are horizontal editable Space Grotesk Bold TEXT.
Original TOERN SVG logo on the left, left edge x=5.25 mm, 26 mm high.
Logo rotation remains 270 degrees total, its latest orientation.

PORT LAYOUT
DIN right: centre (161.75,20.5); flange 31 x 26 mm, right edge x=177.25 mm.
No DIN text. DIN opening 20 mm; 3 mm screw holes at diagonal offsets +/-12 and +/-9.5 mm.
Ports shifted 18 mm left: MIDI/CLOCK/SIDE at x=36,49,62 mm, rows y=28 and13 mm.
Pair labels centered between rows at y=20.5 mm; shared IN/OUT at x=25.5 mm.
Three 11 mm audio holes share y=18 mm: PHONES x=83, LINE-OUT x=101, LINE-IN x=119.
Audio labels centered above holes at y=28.5 mm.
Current vertical auxiliary slot: width11 x height12 mm, centre (136.5,18).
REQUEST PENDING: requested 13 mm width x 70 mm height is NOT applied; it does not clear the LED stack within 36 HP. Current slot retained until placement is decided.
Six 7.8 mm jacks: MIDI IN/OUT, CLOCK IN/OUT, SIDECHAIN and an unassigned lower hole.
Four rack slots 5.5 x 3.2 mm; four matching LED clamp holes 2.2 mm.
Reserved port clearances, DIN flange and rack screw head travel checked for overlap.

PCB MOUNTING - FLIPPED ORIENTATION
Flat PCB underside faces the panel; component side faces into the case.
PCB is rotated 180 degrees about its long X axis; all five baseplate mounting holes are mirrored vertically to match.
Five 2.2 mm M2 holes are verified against the CURRENT KiCad board and regenerated STEP.
See reference/PCB_MOUNTING_HOLES.csv. Use these updated coordinates, not the previous unflipped pattern.
The five PCB screw heads lie beneath the LED: use flush-head mounting or provide an LED spacer gap.
Rear standoffs are illustrated as 4 mm OD / 2.2 mm ID and 19.205 mm long. Length remains a visualization reference.
With this spacing, modeled components extend about 35.95 mm behind the panel's front face.
Available component models only: ten obsolete/custom model references were missing when generating the STEP.
Actual rear component clearances, rail clearances, sheet thicknesses, spacer heights and screw lengths must be checked.
Illustrative stack: base 2 mm, LED 8 mm, front sheet 2 mm. LED is ABOVE the base and BELOW the front sheet.

Clearance guides are separate in reference/modular_clearances.dxf; excluded from the two laser DXFs.
