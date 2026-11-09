// Fit the FlexSPI2 (PSRAM) MPU region to the PSRAM that is actually fitted.
//
// The stock Teensy 4.1 core maps 0x70000000 as a 32 MB cacheable region, but a
// board with 2 x 8 MB PSRAM only has memory behind the first 16 MB. The
// Cortex-M7 speculatively prefetches from any address that happens to sit in a
// register, and a speculative line fill from the unbacked upper half makes
// FlexSPI2 wait for its AHB bus timeout: 4 x 2^28 = 2^30 CPU cycles
// (1.3 s at 816 MHz) during which the whole CPU, every interrupt included,
// is frozen. On TŒRN this showed up as random 1.3 s playback/LED stalls that
// no firmware code path could account for.
//
// Shrinking the region to the detected size makes the MPU reject the
// speculative access immediately, so FlexSPI2 is never asked for it.
// See https://forum.pjrc.com/index.php?threads/78171/ for the analysis.
//
// startup_middle_hook() runs right after configure_external_ram() has probed
// the PSRAM and before USB, C++ constructors and setup().
#include <Arduino.h>

extern "C" uint8_t external_psram_size;

extern "C" FLASHMEM void startup_middle_hook(void) {
  // Nothing hangs on FlexSPI2 port B on a Teensy 4.1; size those slots to zero
  // so an access there fails fast instead of waiting for the AHB timeout.
  FLEXSPI2_FLSHB1CR0 = 0;
  FLEXSPI2_FLSHB2CR0 = 0;

  const uint32_t mb = external_psram_size;
  uint32_t sizeField;
  if (mb == 0 || mb > 16) return;  // no PSRAM or 32 MB: stock region already fits
  sizeField = (mb <= 8) ? 22u : 23u;  // SIZE field: 22 = 8 MB, 23 = 16 MB

  int region = -1;
  uint32_t rasr = 0;
  for (int r = 0; r < 16; ++r) {
    SCB_MPU_RNR = (uint32_t)r;
    asm volatile("dsb" ::: "memory");
    const uint32_t base = SCB_MPU_RBAR & 0xFFFFFFE0u;
    const uint32_t attr = SCB_MPU_RASR;
    if (base == 0x70000000u && (attr & SCB_MPU_RASR_ENABLE)) { region = r; rasr = attr; break; }
  }
  if (region < 0) return;
  const uint32_t updated = (rasr & ~(0x1Fu << 1)) | (sizeField << 1);
  if (updated == rasr) return;

  __disable_irq();
  const uint32_t ctrl = SCB_MPU_CTRL;
  asm volatile("dmb" ::: "memory");
  SCB_MPU_CTRL = 0;
  SCB_MPU_RNR = (uint32_t)region;
  SCB_MPU_RASR = updated;
  SCB_MPU_CTRL = ctrl;
  asm volatile("dsb" ::: "memory");
  asm volatile("isb" ::: "memory");
  __enable_irq();
}
