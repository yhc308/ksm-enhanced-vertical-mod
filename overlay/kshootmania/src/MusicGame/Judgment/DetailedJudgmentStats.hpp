#pragma once
#include <cmath>
#include "JudgmentDefines.hpp"

namespace MusicGame::Judgment
{
    enum class DetailedJudgment { kFastError, kFastNear, kFastCritical, kPerfectCritical,
        kSlowCritical, kSlowNear, kSlowError };

    // An additional classification. Never fed back into score, gauge or AutoSync.
    inline DetailedJudgment ClassifyDetailedJudgment(JudgmentResult result, Optional<double> ms)
    {
        const bool measured = ms && std::isfinite(*ms);
        switch (result)
        {
        case JudgmentResult::kNearFast: return DetailedJudgment::kFastNear;
        case JudgmentResult::kNearSlow: return DetailedJudgment::kSlowNear;
        case JudgmentResult::kError:
            return measured && *ms > 0 ? DetailedJudgment::kFastError : DetailedJudgment::kSlowError;
        case JudgmentResult::kCritical:
            // AUTO, long and laser successes use the highest accuracy bucket.
            if (!measured) return DetailedJudgment::kPerfectCritical;
            if (std::abs(*ms) <= kPerfectCriticalWindowMs) return DetailedJudgment::kPerfectCritical;
            return *ms > 0 ? DetailedJudgment::kFastCritical : DetailedJudgment::kSlowCritical;
        default: return DetailedJudgment::kPerfectCritical;
        }
    }

    struct DetailedJudgmentStats
    {
        int32 fastError = 0, fastNear = 0, fastCritical = 0, perfectCritical = 0;
        int32 slowCritical = 0, slowNear = 0, slowError = 0;

        void add(JudgmentResult result, Optional<double> ms = none)
        {
            if (result == JudgmentResult::kUnspecified) return;
            switch (ClassifyDetailedJudgment(result, ms))
            {
            case DetailedJudgment::kFastError: ++fastError; break;
            case DetailedJudgment::kFastNear: ++fastNear; break;
            case DetailedJudgment::kFastCritical: ++fastCritical; break;
            case DetailedJudgment::kPerfectCritical: ++perfectCritical; break;
            case DetailedJudgment::kSlowCritical: ++slowCritical; break;
            case DetailedJudgment::kSlowNear: ++slowNear; break;
            case DetailedJudgment::kSlowError: ++slowError; break;
            }
        }
    };
}
