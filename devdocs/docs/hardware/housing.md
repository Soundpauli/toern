---
sidebar_position: 6
title: Housing — laser cutting
description: Housing DXF files, the Fusion source project, material, kerf compensation, and the test-cut workflow.
---

# Housing — laser cutting

The housing is made from **3 mm PLEXIGLAS® LED, Black & White 9H04 SC**. Manufacture the flat panels by laser cutting; the DXFs are the cutting and engraving artwork. All files are in the repository-root [`housing/` folder](https://github.com/Soundpauli/toern/tree/main/housing).

## Choose the files

| File | Contents / use |
|------|----------------|
| [`16_16.dxf`](https://github.com/Soundpauli/toern/blob/main/housing/16_16.dxf) | Version-specific parts copied from the **M1** sketch. |
| [`32_16.dxf`](https://github.com/Soundpauli/toern/blob/main/housing/32_16.dxf) | Version-specific parts copied from the shortened sketch. |
| [`COMMON.dxf`](https://github.com/Soundpauli/toern/blob/main/housing/COMMON.dxf) | Three shared parts: the TŒRN logo panel, the four-hole strip, and the smallest strip. |
| [`TestStueck.dxf`](https://github.com/Soundpauli/toern/blob/main/housing/TestStueck.dxf) | The **TestStück** sketch from the TESTS group; cut this first to check the fit and laser settings. |
| [`Fusion360_project.zip`](https://github.com/Soundpauli/toern/blob/main/housing/Fusion360_project.zip) | Complete editable Fusion project; unpack and open the `.f3d` file in Fusion. |
| [`toern_final_v3_v75.f3d`](https://github.com/Soundpauli/toern/blob/main/housing/toern_final_v3_v75.f3d) | The same native Fusion project without the outer ZIP. |

For **one 16_16 housing**, import `16_16.dxf` and `COMMON.dxf`. For **one 32_16 housing**, import `32_16.dxf` and `COMMON.dxf`. Include the COMMON parts once per housing. The version files deliberately exclude those shared parts.

The native project preserves the original sketches as well as **VERSIONS → COMMON / 16_16 / 32_16**. The archive above is the v75 export. When editing the design, regenerate the affected DXFs and update the native archive together.

## Material

| Setting | Specification |
|---------|---------------|
| Material | **PLEXIGLAS® LED, Black & White 9H04 SC** |
| Thickness / Dicke | **3 mm** |
| Exterior contour compensation | **0.125 mm outward / nach AUSSEN** |
| DXF units | **Millimeters**, imported at **1:1** |

9H04 SC has the Black & White effect: black in incident light and white when backlit. The SC sheet is satin-finished on one side; the manufacturer normally places the satin side inward. Check the intended exterior face and lettering orientation before nesting or engraving. See the [manufacturer's product page](https://www.plexiglas.de/en/products/plexiglas/plexiglas-led) and [technical information](https://www.plexiglas.de/files/plexiglas-content/pdf/technische-informationen/222-27-DE-PLEXIGLAS-LED-Hinterleuchtung-Farbwechselnd.pdf).

## Kerf compensation

Use **0.125 mm outward compensation on exterior cutting contours**. The DXFs contain nominal geometry; no kerf offset has been baked into them. Apply the compensation once in the laser software, rather than also offsetting the DXF geometry.

For holes and other internal cutouts, the toolpath compensation must point into the removed area so the finished opening keeps its nominal size. Check the software's handling of nested contours in its toolpath preview; do not apply an outward enlargement to every hole independently. Engraving artwork receives no cutting offset.

The specified **0.125 mm is the requested contour offset**, not a measurement of the full laser-cut width. Software fields named “kerf width” and “kerf offset” can mean different things. For example, LightBurn uses a positive Kerf Offset for outward adjustment and explains that a measured full kerf width is divided by two to obtain an offset. Use the [LightBurn kerf guide](https://docs.lightburnsoftware.com/2.2/Projects/laser-cut-dinosaur/) to interpret that field; do not halve the specified project offset just because it is called kerf here.

## Cut and assemble

1. Import `TestStueck.dxf` in millimeters at **100% scale**. Use the same sheet, machine profile, and kerf compensation intended for the housing.
2. Cut the test piece and check the finger-joint fit against the actual 3 mm material. Confirm the parts seat without forcing them. Adjust the machine settings or compensation based on this test before cutting the full set.
3. Import **COMMON plus the chosen version**. Check scale against the native design: the largest rectangular panel is 246 mm wide in `16_16` and 166 mm wide in `32_16`.
4. Put lettering and the logo on an **engraving** operation. Put panel outlines, holes, and slots on **cutting** operations. Construction guides and sketch points are excluded from the exports.
5. Check for duplicate paths, closed cutting contours, lettering orientation, and adequate spacing between panels. Nest parts by moving or rotating whole panels without scaling them.
6. Engrave first, cut internal openings next, and cut external panel outlines last. Use a machine-specific profile for this material; speed and power values are not specified by these files.
7. Dry-fit the panels and check connectors and the PCB before final assembly. Orient the satin face and labels consistently with the intended exterior.

For a paper layout check, print at **actual size / 100%**, with “fit to page” disabled. A paper print does not replace the material test cut.
