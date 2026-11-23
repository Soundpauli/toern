# RevJ 3D models

These files are linked from the revJ PCB using project-relative paths. Existing project-local models for the 3.5 mm jacks, switches, charger, Teensy sockets and pogo contacts retain their original links.

| References | Source and model placement |
|---|---|
| J1, J5–J7 | Neutrik NMJ6HCD2 STEP. Aligned to the two PCB pin rows and the panel-facing axis. [Neutrik CAD download](https://www.neutrik.com/en/product/nmj6hcd2) |
| J4, J33 | HRO TYPE-C-31-M-12 catalog STEP. Aligned to the shell mounting rows. [HRO part drawing](https://www.krhro.com/Product-Details/726.html) |
| J8 | DM3BT-DSF-PEJS catalog STEP. Aligned to the eight contacts and shell lands. [Hirose part drawing](https://www.hirose.com/en/product/p/CL0609-0029-9-00) |
| U3 | SGTL5000XNBA3 catalog package model. The Z scale represents the nominal 0.85 mm package height. [NXP package drawing](https://www.nxp.com/packages/SOT617-27) |
| U4 | TLP2361 catalog STEP, rotated to match pin 1. [Toshiba package data](https://toshiba.semicon-storage.com/eu/semiconductor/design-development/eda-cad-lib/3d-step-library.html) |
| U6 | Zack Kummer’s Teensy 4.1 model from XenGi’s library, with the board’s existing 7 mm model offset for the socketed assembly. [Teensy library](https://github.com/XenGi/teensy.pretty) |
| U8 | Existing project-local TPS22918DBVR STEP, rotated upright and aligned to pin 1. |
| Standard parts | Stock KiCad models, using STEP where the same library model is available in that format. [KiCad 3D models project](https://gitlab.com/kicad/libraries/kicad-packages3D) |

The PCB keeps its KiCad 9 file format. A comparison of the saved board against the baseline matched every non-model line. The model path check and KiCad renders used only project-local assets. Eleven existing socket and pogo models use VRML and appear in KiCad’s 3D view. KiCad’s STEP exporter omits them.
