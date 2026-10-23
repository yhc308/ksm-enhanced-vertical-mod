#pragma once
#include "ksmaudio/Stream.hpp"
namespace MusicGame::Audio {
// EOF state is authoritative even when mixer history ends short of the duration.
inline bool ShouldStartTailClock(const ksmaudio::AudioClockSnapshot& clock,double endChartSec,double sampleRate) {
    return clock.valid && sampleRate>0 &&
        (!clock.playing || clock.renderPositionSec>=endChartSec-1.0/sampleRate);
}
}
