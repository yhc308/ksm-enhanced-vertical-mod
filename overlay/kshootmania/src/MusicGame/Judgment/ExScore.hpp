#pragma once
#include "DetailedJudgmentStats.hpp"
#include "kson/ChartData.hpp"
namespace MusicGame::Judgment {
inline int64 ExChipPoints(JudgmentResult result,Optional<double> ms=none) {
    if(result==JudgmentResult::kCritical)
        return ClassifyDetailedJudgment(result,ms)==DetailedJudgment::kPerfectCritical?5:4;
    return result==JudgmentResult::kNearFast || result==JudgmentResult::kNearSlow?2:0;
}
inline int64 ExTickPoints(JudgmentResult result){return result==JudgmentResult::kCritical?2:0;}
int64 ChartExMaximum(const kson::ChartData& chart);
int64 LoadChartExMaximum(FilePathView path);
inline int ExPanelKind(int64 score,int64 maximum){
    if(maximum<=0 || score<0 || score>maximum)return 0;
    if(score==maximum)return 3;
    return score*100>maximum*98?2:1;
}
inline int64 ExPercentHundredths(int64 score,int64 maximum){return maximum>0?score*10000/maximum:0;}
}
