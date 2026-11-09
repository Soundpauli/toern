TOERN modular - 36 HP

modular.dxf: base panel 182.5 x 128.5 mm.
modular_front.dxf: front clamping sheet 172 x 86 mm.
modular_2parts.dxf: both sheets in one file, separate groups, 4 mm gap.
Millimetres, 1:1. SCHNITT = cut; GRAVUR = engrave.
Functional labels are horizontal editable Space Grotesk Bold TEXT.
Original TOERN SVG logo on the left, left edge x=5.25 mm, 26 mm high.
Logo rotation remains 270 degrees total, its latest orientation.

PORT LAYOUT AND MARKINGS
Current Fusion source: toern_final_v3_v118 v1. Engraving preserved; screw and audio clearances updated.
DIN right, logo left; no DIN text.
MIDI A = TRS MIDI Type A (tip DIN5 / current sink; ring DIN4 / current source).
CLCK and SC are the abbreviated clock/sidechain headings.
Functional labels remain editable Space Grotesk Bold TEXT.
IN/OUT words replaced by horizontal arrow/bar icons, 0.45 mm rounded strokes:
upper row points right toward the bar (IN); lower row points left away from it (OUT).
Icons are fixed engraving outlines on GRAVUR, excluded from SCHNITT.
Three audio socket cutouts enlarged from diameter 11 to 11.5 mm. Other ports unchanged.
The earlier 70 x 13 mm slot request remains pending placement; it is not applied here.

FRONT CLAMP MOUNTING - M2.5
Front sheet: four diameter 2.8 mm clearance holes for M2.5 screws.
Base sheet: matching horizontal 5 x 2.8 mm rounded slots, allowing lateral compensation.
Hole/slot centers: x=8.75 or 173.75; y=39.5 or 118.5 mm in base coordinates.
Front holes leave 2.1 mm material to each nearest cover edge.
Reference screw-head envelopes: diameter 6 mm at cover; 8.2 x 6 mm swept area at base.
PCB mounts remain M2; do not use the larger clamp screws in the PCB holes.

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
