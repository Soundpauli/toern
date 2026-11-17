#pragma once
#include <stdint.h>

// Teensy USB-host Launchpad mixer. Safe to call when TOERN_LAUNCHPAD_HOST=0 (stubs).
void launchpadHostBegin();
void launchpadHostService();

// Bridge helpers implemented in toern.ino (avoid pulling GlobalVars into this TU).
bool launchpadGetChildLock();
unsigned launchpadGetCurrentChannel();
void launchpadSelectVoice(unsigned ch);
unsigned launchpadGetChannelVolume(unsigned ch);
void launchpadSetChannelVolume(unsigned ch, int vol);
bool launchpadGetMuted(unsigned ch);
void launchpadSetMuted(unsigned ch, bool muted);
uint32_t launchpadGetVoiceColor(unsigned ch);  // active color scheme, 0xRRGGBB
