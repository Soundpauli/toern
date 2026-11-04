"""Idempotent, fail-closed Teensy 4 USB TX fix (PlatformIO framework patch)."""
MARKER = '// TOERN_USB_TX_V1'

def patch_usb_audio(text):
    if MARKER in text:
        return text
    def replace(old, new):
        nonlocal text
        if text.count(old) != 1:
            raise RuntimeError('Unsupported Teensy USB audio source: ' + old[:70])
        text = text.replace(old, new, 1)
    replace('volatile uint32_t usb_audio_overrun_count;', '''volatile uint32_t usb_audio_overrun_count;
// TOERN_USB_TX_V1: device-to-host counters, separate from AudioInputUSB.
volatile uint32_t toern_usb_tx_discarded_frames = 0;
volatile uint32_t toern_usb_tx_overruns = 0;
volatile uint32_t toern_usb_tx_silent_frames = 0;
volatile uint32_t toern_usb_tx_underruns = 0;''')
    replace('// buffer overrun - PC is consuming too slowly', '''// buffer overrun - PC is consuming too slowly
        toern_usb_tx_overruns++;
        toern_usb_tx_discarded_frames += AUDIO_BLOCK_SAMPLES - offset_1st;''')
    replace('memset(usb_audio_transmit_buffer + len, 0, num * 4);', '''// len counts stereo frames, not uint16_t elements. Clear only
            // the unsupplied tail; never overwrite frames already copied.
            memset((uint32_t *)usb_audio_transmit_buffer + len, 0, num * 4);
            toern_usb_tx_underruns++;
            toern_usb_tx_silent_frames += num;''')
    return text
