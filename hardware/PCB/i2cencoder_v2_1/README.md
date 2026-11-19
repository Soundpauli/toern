# DuPPa I2C Encoder V2.1 for TŒRN

Open [encoder.kicad_pro](boards/encoder/encoder.kicad_pro) for the standalone four-layer KiCad project. The PCB preserves the DuPPa module outline, header and encoder drill geometry and adds a component-side TC2030-PKT-NL ICSP connection. See the [layout and validation report](review/layout.md) for the KiCad 9-format export, electrical checks and board renders.

The materials in [extra/upstream](extra/upstream) are pinned to DuPPa commit `d47fd5afe6d1130eee8f7d02c4856071b2adc8be`. They include the original hardware release and upstream license. Firmware source and both published HEX images are available through [pinned upstream links](extra/upstream/Firmware/README.md), rather than redistributed here. [DuPPa repository](https://github.com/DuPPadotnet/I2CEncoderV2.1/tree/d47fd5afe6d1130eee8f7d02c4856071b2adc8be) · [CircuitMaker project](https://workspace.circuitmaker.com/Projects/Details/Simone--Caron/I2C-Encoder-V2)

## Licensing

Hardware and the KiCad/ICSP modifications: **GPLv3**. Documentation: **CC BY-SA 4.0**. Original design by **DuPPa / Simone Caron**; TŒRN adaptations by Jan-Peter Kuntoff, recorded 2026-11-19. This folder is excluded from TŒRN’s MIT and noncommercial hardware defaults and exclusive-commercial-sale restriction. See [NOTICE.md](NOTICE.md) for attribution, modification details and license scope, and [LICENSE](LICENSE) for GPLv3.

## ICSP connector

Use the standard six-pad TC2030 no-legs pattern with the **TC2030-PKT-NL** cable for PICkit 3, 4 or 5. The TC2030-MCP-NL uses the same board geometry and ICSP assignment with an RJ12 programmer end. J1 implements the standard geometry on the component side with the numbered contacts connected as shown below. [Direct PICkit cable](https://www.tag-connect.com/product/tc2030-pkt-nl-6-pin-no-legs-cable-for-microchip-pickit-3) · [Tag-Connect pattern and wiring](https://www.tag-connect.com/wp-content/uploads/bsk-pdf-manager/2022/06/TC2030-MCP-NL.pdf)

| Pad | ICSP signal | DuPPa connection |
|---:|---|---|
| 1 | MCLR / VPP | U1 pin 1, RA3 / ADD3, MCU side of SJ4 |
| 2 | VDD target | U1 pin 18, VDD, MCU side of ferrite L1 |
| 3 | GND | U1 pin 17 and exposed pad GND net |
| 4 | PGD / ICSPDAT | U1 pin 16, RA0 / ADD0, MCU side of SJ1 |
| 5 | PGC / ICSPCLK | U1 pin 15, RA1 / ADD1, MCU side of SJ2 |
| 6 | AUX | Leave open |

J1 has three 0.9906 mm non-plated guide holes, six 0.7874 mm mask-open pads with no paste, and 1.27 mm contact spacing. The PCB has rule areas for the contact gap and nearby pours. The silkscreen dot identifies pin 1. Keep the module off the carrier while attaching the programming cable. [DuPPa hardware schematic and PCB drawing](https://github.com/DuPPadotnet/I2CEncoderV2.1/blob/master/Hardware/EncoderI2Cv2.1Layer.pdf) · [Microchip programming specification](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/ProgrammingSpecifications/40001738D.pdf)

## Flash sequence

A blank PIC16F18345 needs programming. Use MPLAB IPE with device PIC16F18345 and a PICkit, select the upstream [REV2.3 production HEX](https://github.com/DuPPadotnet/I2CEncoderV2.1/blob/d47fd5afe6d1130eee8f7d02c4856071b2adc8be/Firmware/Binary%20file%20REV2.3/I2CEncoderV2.1.X.production.hex), and program and verify flash and configuration words. Program with the module disconnected from the TŒRN carrier and with SJ1, SJ2 and SJ4 open. Power the target from one source and have the programmer reference the target VDD through pad 2. Use high-voltage ICSP, which is required to write the image's disabled LVP configuration. Set the address jumpers only after programming. Reopen any of SJ1, SJ2 or SJ4 that were closed before reprogramming the module.

Both upstream HEX files passed every Intel HEX record checksum and have LVP cleared in CONFIG3. The newer published REV2.3 file is the proposed first-article image. There has been no hardware flash or verification yet. [Upstream firmware notes](https://github.com/DuPPadotnet/I2CEncoderV2.1/tree/master/Firmware) · [Microchip programming specification](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/ProgrammingSpecifications/40001738D.pdf)
