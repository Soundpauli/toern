# Encoder V2.1 with PICkit ICSP

The KiCad reconstruction uses DuPPa’s published Gerbers, plated and non-plated drills, placement file and schematic. The PCB retains the 25.00 × 25.00 mm outline, nominal 2.50 mm corner radii, two Ø3.00 mm mounting holes, rotary encoder holes and plated slots, 20 edge castellations and both five-pin 2.54 mm pitch headers. The internal header rows remain 21.0002 mm apart. Their copper annuli were converted to concentric pads linked to the castellations by short tracks. The original drill centers stayed fixed. Pin and net assignments were cross-checked against the source schematic and the four source copper layers.

J1 uses the TC2030 no-legs pattern on the PIC side. Nominal contact diameter is 0.7874 mm, contact pitch is 1.27 mm, and the three guide holes are Ø0.9906 mm NPTH. The local copper audit found at least 0.517 mm from the contact lands to unrelated routed copper, excluding the five other contact lands defined by the same pattern. The smallest guide-hole-to-copper clearance measured on the exterior layers is 0.253 mm. The pad-gap and pour keepouts are saved as KiCad rule areas. All six contacts have exposed mask openings and no paste. J1.6 is an isolated contact. [Tag-Connect drawing](https://www.tag-connect.com/wp-content/uploads/bsk-pdf-manager/2022/06/TC2030-MCP-NL.pdf)

## Routing

- F.Cu and B.Cu carry the source signal routes and ground pours. In1.Cu is primarily GND and carries ICSP branches. In2.Cu is primarily VCC and also carries ICSP branches. All four pours were rebuilt and refilled in KiCad.
- Added ICSP signal tracks are 0.2032 mm. J1 VDD and GND fanout tracks are 0.25 mm. The new plated vias have 0.30 mm drills and 0.55–0.60 mm diameters. The 0.15 mm clearance rule preserves the source routing's nominal 6 mil spacing. The first article assumes a 1.6 mm, four-layer FR-4 board.
- J1.1 reaches U1.1, J1.2 reaches U1.18 on the MCU side of L1, J1.3 reaches GND, J1.4 reaches U1.16 and J1.5 reaches U1.15. J1.6 is reserved. The address straps and pull-up selector remain part of the upstream circuit. [DuPPa schematic](https://github.com/DuPPadotnet/I2CEncoderV2.1/blob/master/Hardware/EncoderI2Cv2.1Layer.pdf) · [Microchip programming specification](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/ProgrammingSpecifications/40001738D.pdf)

## Validation

- [PCB DRC](pcb-drc.json) found zero violations, zero unrouted connections and zero schematic-parity differences. [Schematic ERC](schematic-erc.json) found zero violations. These reports record the tool’s default ignored checks, including missing footprint courtyards.
- [Exported-copper verification](copper-audit.txt) independently traced 122 terminals across 33 nets, including the three isolated contacts or mounting tabs, through independently plotted copper and plated drill data. It checked 46 preserved component, connector, castellation and mounting drill/slot shapes and verified the solder mask and paste at J1. [ICSP clearance measurements](icsp-clearances.txt)
- [Front render](board-front.png) · [ICSP side render](board-icsp.png) · [Schematic drawing](schematic.svg)
- The delivered PCB was exported to KiCad 9.0's native file format. The schematic and project libraries use KiCad 9-era formats. The compatibility export drops newer per-via covering, plugging, capping and filling flags, which were all unset in this design. The CAD checks above were run with Backplane KiCad on the delivered files. A launch in stock KiCad 9.0.5, physical probe fit, assembly and programming a real PIC remain untested.

The original BOM lists the reflow components. The RGB encoder, optional external RGB LED and headers are hand-assembly hardware. Confirm the chosen parts against the original drawings before purchasing. Use the [programming sequence](../README.md#flash-sequence) with the encoder off the carrier.
