#pragma once

// Hardware selector: the default is capacitive; teensy41-modulr overrides it.
// Set TOERN_EXT_TOUCH when building for another unit:
// false: capacitive pads, no faders, standard 100 kHz I2C.
// true:  grounded buttons A–D, four NeoSliders, and approximately 35 kHz I2C
//        for the remote panel. Encoder addresses stay 0x01, 0x41, 0x20, 0x61.
#ifndef TOERN_EXT_TOUCH
#define TOERN_EXT_TOUCH 0
#endif
static constexpr bool exttouch = TOERN_EXT_TOUCH != 0;

// Current unit: fader 2 / voice 2 at 0x31 is broken. No probe, ADC or LED I/O.
// Clear bit 1 after replacing the hardware to enable it again.
static constexpr unsigned int neoSliderDisabledMask = 1u << 1;
