#pragma once
#include <cstddef>
namespace toern_audio {
// FILTER_WAVEFORM was serialized by accessing column 15 of a 15-column row.
// Explicitly address those same legacy bytes through their real array members.
// This retains the Device binary layout and existing saved pattern values.
template<std::size_t Rows, std::size_t Columns, std::size_t SynthColumns>
float& legacyFilterValue(float (&filters)[Rows][Columns], float (&synth)[Rows][SynthColumns],
                         int channel, int index) {
    static float ignored = 0;
    if (channel < 0 || channel >= static_cast<int>(Rows) || index < 0) return ignored;
    if (index < static_cast<int>(Columns)) return filters[channel][index];
    if (index == static_cast<int>(Columns)) {
        if (channel + 1 < static_cast<int>(Rows)) return filters[channel + 1][0];
        return synth[0][0];
    }
    return ignored;
}
}
