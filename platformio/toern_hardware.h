#pragma once

// Single hardware selector. Change only this flag when choosing a unit:
// false: capacitive pads, no faders, standard 100 kHz I2C.
// true:  grounded buttons A–D, four NeoSliders, and approximately 35 kHz I2C
//        for the remote panel. Encoder addresses stay 0x01, 0x41, 0x20, 0x61.
static constexpr bool exttouch = false;

// Current unit: fader 2 / voice 2 at 0x31 is broken. No probe, ADC or LED I/O.
// Clear bit 1 after replacing the hardware to enable it again.
static constexpr unsigned int neoSliderDisabledMask = 1u << 1;
