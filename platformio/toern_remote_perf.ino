#if TOERN_REMOTE_CONTROL
extern volatile uint32_t toern_usb_tx_discarded_frames __attribute__((weak));
extern volatile uint32_t toern_usb_tx_overruns __attribute__((weak));
extern volatile uint32_t toern_usb_tx_silent_frames __attribute__((weak));
extern volatile uint32_t toern_usb_tx_underruns __attribute__((weak));
extern "C" uint8_t external_psram_size;
// On-demand telemetry: AudioStream keeps these cycle counters already. Reading
// them does not walk/mutate the graph, redraw the UI, or pause playback.
FLASHMEM static unsigned remoteCpuBasisPoints(float percent) {
  return (unsigned)lroundf(max(0.0f, percent) * 100.0f);
}

FLASHMEM void remoteReplyPerformance(unsigned long id) {
  remoteReplyStart(id);
  extern volatile uint32_t toern_audio_peak_cycles,toern_audio_peak_node_cycles,toern_audio_peak_node,toern_audio_peak_vtable;
  Serial.printf("\"audio_raw_cycles\":[%lu,%lu,%lu,%lu],",(unsigned long)toern_audio_peak_cycles,(unsigned long)toern_audio_peak_node_cycles,(unsigned long)toern_audio_peak_node,(unsigned long)toern_audio_peak_vtable);
  Serial.print("\"detail_us\":[");for(unsigned i=0;i<8;++i)Serial.printf("%s%lu",i?",":"",(unsigned long)detailMax[i]);Serial.print("],");
  {
    using namespace toern_stall_probe;
    const uint32_t n = recordCount;
    Serial.printf("\"stall_probe_count\":%lu,\"stall_probe\":[", (unsigned long)n);
    const unsigned shown = n < kRecords ? n : kRecords;
    for (unsigned i = 0; i < shown; ++i) {
      const volatile Record &r = records[i];
      Serial.printf("%s{\"ms\":%lu,\"stuck_ms\":%lu,\"pc\":\"0x%08lx\",\"lr\":\"0x%08lx\",\"xpsr\":\"0x%08lx\",\"r0\":\"0x%08lx\",\"r1\":\"0x%08lx\"}",
          i ? "," : "", (unsigned long)r.ms, (unsigned long)r.stuckMs, (unsigned long)r.pc, (unsigned long)r.lr,
          (unsigned long)r.xpsr, (unsigned long)r.r0, (unsigned long)r.r1);
    }
    Serial.print("],");
  }
  {
    uint32_t sizeField = 0;
    for (int r = 0; r < 16; ++r) {
      SCB_MPU_RNR = (uint32_t)r; asm volatile("dsb" ::: "memory");
      if ((SCB_MPU_RBAR & 0xFFFFFFE0u) == 0x70000000u && (SCB_MPU_RASR & 1u)) { sizeField = (SCB_MPU_RASR >> 1) & 0x1Fu; break; }
    }
    Serial.printf("\"psram_mb\":%u,\"flexspi2_mpu_mb\":%lu,", (unsigned)external_psram_size,
        sizeField ? (unsigned long)(1ull << (sizeField + 1)) >> 20 : 0ul);
  }
  Serial.printf("\"playback_calls\":%lu,\"playback_max_us\":%lu,\"playback_gap_us\":%lu,\"playback_late\":%lu,\"guard_max_us\":%lu,",(unsigned long)playbackCalls,(unsigned long)playbackMaxUs,(unsigned long)playbackGapUs,(unsigned long)playbackLate,(unsigned long)toern_audio::guardMaxUs);
  if (&toern_usb_tx_discarded_frames && &toern_usb_tx_overruns &&
      &toern_usb_tx_silent_frames && &toern_usb_tx_underruns) {
    Serial.printf("\"usb_tx\":{\"discarded_frames\":%lu,\"overruns\":%lu,\"silent_frames\":%lu,\"underruns\":%lu},",
      (unsigned long)toern_usb_tx_discarded_frames, (unsigned long)toern_usb_tx_overruns,
      (unsigned long)toern_usb_tx_silent_frames, (unsigned long)toern_usb_tx_underruns);
  } else {
    Serial.print("\"usb_tx\":null,"); // Unpatched Arduino IDE core: diagnostics unavailable.
  }
  Serial.printf("\"i2c_errors\":%lu,\"i2c_max_us\":%lu,\"i2c_skipped\":%lu,",(unsigned long)toernI2cErrors(),(unsigned long)toernI2cMaxUs(),(unsigned long)toernI2cSkipped());
  Serial.printf("\"temp_centi\":%d,\"loop_max_us\":%lu,\"loop_stalls\":%lu,\"last_stall_us\":%lu,\"last_stall_stage\":\"%s\",\"stage_max_us\":{",
      (int)(tempmonGetTemp()*100.0f),(unsigned long)loopTiming.loopMaximum,
      (unsigned long)loopTiming.stalls,(unsigned long)loopTiming.lastStallUs,ToernLoopTiming::name(loopTiming.lastStallStage));
  for(unsigned i=0;i<ToernLoopTiming::Count;++i)Serial.printf("%s\"%s\":%lu",i?",":"",ToernLoopTiming::name((ToernLoopTiming::Stage)i),(unsigned long)loopTiming.maximum[i]);
  Serial.print("},");
  Serial.printf("\"ok\":true,\"reverb_quality\":%u,\"ch11_internal_ram\":%s,\"input_recoveries\":%lu,\"cpu_bp\":%u,\"cpu_max_bp\":%u,\"audio_blocks\":%u,\"audio_blocks_max\":%u,\"cpu_hz\":%lu,\"playing\":%s,\"ch11_muted\":%s,\"ch11_active\":[",
      (unsigned)TOERN_TREEVERB_QUALITY,
      ((uintptr_t)Sladder1[0] >= 0x20000000u && (uintptr_t)Sladder1[0] < 0x20200000u) ? "true" : "false",
      (unsigned long)remoteInputRecoveries,
      remoteCpuBasisPoints(AudioProcessorUsage()), remoteCpuBasisPoints(AudioProcessorUsageMax()),
      (unsigned)AudioMemoryUsage(), (unsigned)AudioMemoryUsageMax(), (unsigned long)F_CPU_ACTUAL,
      isNowPlaying ? "true" : "false", getMuteState(11) ? "true" : "false");
  for (int i = 0; i < 3; ++i) Serial.printf("%s%s", i ? "," : "",
      (Senvelope1[i]->isActive() || Senvelope2[i]->isActive()) ? "true" : "false");
  Serial.print("],\"ladder_bp\":[");
  for (int i = 0; i < 3; ++i) Serial.printf("%s%u,%u", i ? "," : "",
      remoteCpuBasisPoints(Sladder1[i]->processorUsage()), remoteCpuBasisPoints(Sladder2[i]->processorUsage()));
  Serial.print("],\"oscillator_bp\":[");
  for (int i = 0; i < 3; ++i) Serial.printf("%s%u,%u,%u", i ? "," : "",
      remoteCpuBasisPoints(Swaveform1[i]->processorUsage()), remoteCpuBasisPoints(Swaveform2[i]->processorUsage()),
      remoteCpuBasisPoints(Swaveform3[i]->processorUsage()));
  Serial.printf("],\"ch11_reverb_bp\":%u,\"ch11_delay_bp\":%u,\"ch11_filter_bp\":%u,\"ch11_bitcrusher_bp\":%u}\n",
      remoteCpuBasisPoints(freeverb11.processorUsage()), remoteCpuBasisPoints(voiceDelay[8].processorUsage()),
      remoteCpuBasisPoints(filter11.processorUsage()), remoteCpuBasisPoints(bitcrusher11.processorUsage()));
}

#endif
