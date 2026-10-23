#pragma once
#include "Common/FsUtils.hpp"
#include "Ini/ConfigIni.hpp"
#include "MusicGame/TimingFeedback.hpp"
// Explicit, fixture-only visual regression inputs. Never enabled during regular play.
namespace TimingHudCapture {
inline bool Choices() { static const bool value=System::GetCommandLineArgs().contains(U"--timing-choices-capture");return value; }
inline bool IsMenu() { static const bool value=System::GetCommandLineArgs().contains(U"--timing-menu-capture");return value || Choices(); }
inline bool Enabled() {
    static const bool value=[] {
        const bool requested=IsMenu() || System::GetCommandLineArgs().contains(U"--timing-hud-capture");
        if (requested && !FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture")))
            throw Error{U"Visual captures require an isolated fixture directory"};
        return requested;
    }();return value;
}
inline Stopwatch& Clock() { static Stopwatch value;return value; }
inline bool& Ready() { static bool value=false;return value; }
inline void Begin() { if (!Ready()) {Clock().restart();Ready()=true;} }
inline int32 Stage() {return Clamp(static_cast<int32>(Clock().sF()/1.2),0,Choices()?3:IsMenu()?2:9);}
inline double Fraction() {return std::fmod(Clock().sF(),1.2);}
inline bool Done() {return Clock().sF()>1.2*(Choices()?4:IsMenu()?3:10);}
inline MusicGame::TimingFeedback Event(double now) {
    using namespace MusicGame::Judgment;
    const auto stage=Stage();
    if(stage==6) return {JudgmentResult::kCritical,3.2,now,MusicGame::TimingWordFeedback{-60.3,now-0.2}};
    return {stage==4 ? JudgmentResult::kNearSlow : stage==5 || stage==7 ? JudgmentResult::kCritical
        : stage==9 ? JudgmentResult::kError : JudgmentResult::kNearFast,
        stage==4 ? -47.8 : stage==5 ? -9.5 : stage==7 ? 21.1 : stage==9 ? 130.0 : 60.3,now};
}
inline MusicGame::TimingHudSettings Settings() {
    MusicGame::TimingHudSettings value;
    value.judgeColor=ConfigIni::GetInt(ConfigIni::Key::kJudgeColor,0)==1?JudgeColor::kFastRed:JudgeColor::kFastBlue;
    value.position=static_cast<TimingPosition>(Stage()<4 ? Stage() : 0);
    value.display=Stage()==7 ? TimingMsDisplay::kCrit : TimingMsDisplay::kAll;
    value.color=Stage()==5 || Stage()==7 ? TimingMsColor::kCrit : Stage()==6 || Stage()==9 ? TimingMsColor::kOff : TimingMsColor::kNear;
    return value;
}
}
