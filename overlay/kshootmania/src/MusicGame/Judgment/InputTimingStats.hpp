#pragma once
#include <Siv3D.hpp>
#include "JudgmentDefines.hpp"

namespace MusicGame::Judgment {
// One population for the original result Average and the live indicator.
// No miss penalty or additional timing-window filter is applied.
struct InputTimingStats {
    uint64 count = 0;
    double sumMs = 0, absoluteSumMs = 0;

    void add(JudgmentResult result, Optional<double> ms = none) {
        if (!ms || !std::isfinite(*ms)) return;
        if (result != JudgmentResult::kCritical && result != JudgmentResult::kNearFast
            && result != JudgmentResult::kNearSlow) return;
        ++count; sumMs += *ms; absoluteSumMs += std::abs(*ms);
    }
    double meanMs() const { return count ? sumMs / count : 0.0; }
    double absoluteMeanMs() const { return count ? absoluteSumMs / count : 0.0; }
    Optional<int32> roundedMeanMs() const {
        return count ? Optional<int32>{static_cast<int32>(Round(meanMs()))} : none;
    }
};
}
