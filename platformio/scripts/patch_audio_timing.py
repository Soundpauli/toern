"""Temporary raw audio timing for the playback-stall investigation."""
REPLACEMENTS = [('void software_isr(void) // AudioStream::update_all()\n', 'volatile uint32_t toern_audio_peak_cycles=0,toern_audio_peak_node_cycles=0,toern_audio_peak_node=0,toern_audio_peak_vtable=0;\nvoid software_isr(void) // AudioStream::update_all()\n'), ('\t\t\tcycles = (ARM_DWT_CYCCNT - cycles) >> 6;\n', '\t\t\tcycles = ARM_DWT_CYCCNT - cycles;\n            if(cycles>toern_audio_peak_node_cycles){toern_audio_peak_node_cycles=cycles;toern_audio_peak_node=(uint32_t)p;toern_audio_peak_vtable=*(uint32_t*)p;}\n            cycles >>= 6;\n'), ('\ttotalcycles = (ARM_DWT_CYCCNT - totalcycles) >> 6;\n', '\ttotalcycles = ARM_DWT_CYCCNT - totalcycles;\n    if(totalcycles>toern_audio_peak_cycles)toern_audio_peak_cycles=totalcycles;\n    totalcycles >>= 6;\n')]

def patch_audio_timing(text):
    if 'volatile uint32_t toern_audio_peak_cycles=' in text:
        return text
    for old, new in REPLACEMENTS:
        if text.count(old) != 1:
            raise RuntimeError('Unsupported AudioStream source for stall diagnostics')
        text = text.replace(old, new, 1)
    return text
