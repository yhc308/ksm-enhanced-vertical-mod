#pragma once
#include "TimingHudCapture.hpp"
#include "MusicGame/ViewStatus.hpp"
#include "Common/SkinPreset.hpp"

// Explicit, isolated visual inputs. These never change the game's actual judgments or result.
namespace PlayJudgeIndicatorFixture {
inline void Configure() {
    auto settings=SkinPreset::Current();const auto& args=System::GetCommandLineArgs();
    if(args.contains(U"--indicator-both"))settings.indicatorPortrait=SkinPreset::IndicatorPortrait::Both;
    if(args.contains(U"--indicator-off")){settings.indicatorPortrait=SkinPreset::IndicatorPortrait::Off;settings.indicatorLandscape=false;}
    SkinPreset::Activate(settings);
}
inline MusicGame::ViewStatus View(const MusicGame::ViewStatus& original) {
    auto view = original;
    using namespace MusicGame::Judgment;
    view.detailedJudgments = {12,56,90,1234,67,78,44};
    view.inputTiming = {};
    switch (TimingHudCapture::Stage()) {
    case 4:
        view.inputTiming.add(JudgmentResult::kCritical,-15.5);
        view.inputTiming.add(JudgmentResult::kCritical,9.9);
        break;
    case 5:
        view.inputTiming.add(JudgmentResult::kCritical,-0.01);
        break;
    case 6:
        view.detailedJudgments = {999999,999999,999999,999999,999999,999999,999999};
        view.inputTiming.add(JudgmentResult::kCritical,123.456);
        break;
    default:
        view.inputTiming.add(JudgmentResult::kCritical,15.5);
        view.inputTiming.add(JudgmentResult::kCritical,-9.9);
    }
    return view;
}
}
