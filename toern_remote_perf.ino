#if TOERN_REMOTE_CONTROL
// On-demand telemetry: AudioStream keeps these cycle counters already. Reading
// them does not walk/mutate the graph, redraw the UI, or pause playback.
FLASHMEM static unsigned remoteCpuBasisPoints(float percent) {
  return (unsigned)lroundf(max(0.0f, percent) * 100.0f);
}

FLASHMEM void remoteReplyPerformance(unsigned long id) {
  remoteReplyStart(id);
  Serial.printf("\"ok\":true,\"ch11_internal_ram\":%s,\"input_recoveries\":%lu,\"cpu_bp\":%u,\"cpu_max_bp\":%u,\"audio_blocks\":%u,\"audio_blocks_max\":%u,\"cpu_hz\":%lu,\"playing\":%s,\"ch11_muted\":%s,\"ch11_active\":[",
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
