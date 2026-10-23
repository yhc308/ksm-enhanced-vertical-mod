#pragma once
#include <Siv3D.hpp>
#include <cmath>
#include "Common/CommonDefines.hpp"
#include "Judgment/JudgmentDefines.hpp"

namespace MusicGame {
struct TimingHudSettings {
    JudgeColor judgeColor = JudgeColor::kFastBlue;
    TimingPosition position = TimingPosition::kUpperPlus;
    TimingMsDisplay display = TimingMsDisplay::kCrit;
    TimingMsColor color = TimingMsColor::kCrit;
};
// Saved enum IDs remain stable; menu order is independent of persisted values.
inline constexpr std::array<TimingMsDisplay,4> kTimingDisplayChoices{TimingMsDisplay::kOff,TimingMsDisplay::kNear,TimingMsDisplay::kCrit,TimingMsDisplay::kAll};
inline constexpr std::array<StringView,4> kTimingDisplayLabels{U"OFF",U"NEAR",U"CRIT",U"ALL"};
inline constexpr std::array<StringView,4> kTimingColorLabels{U"OFF",U"NEAR",U"CRIT",U"±16ms"};
inline int32 TimingDisplayCursor(TimingMsDisplay value){for(int32 i=0;i<4;++i)if(kTimingDisplayChoices[i]==value)return i;return 0;}
inline TimingMsDisplay TimingDisplayChoice(int32 cursor){return kTimingDisplayChoices[Clamp(cursor,0,3)];}
inline Color JudgeDirectionColor(JudgeColor palette,bool fast) {
    return fast==(palette==JudgeColor::kFastRed)?Color{220,88,100}:Color{88,132,220};
}
inline Color JudgeTimingColor(JudgeColor palette,double ms) {return ms==0?Color{178,178,178}:JudgeDirectionColor(palette,ms>0);}
inline String JudgeImage(StringView base,JudgeColor palette) {
    String name{base};if(palette==JudgeColor::kFastRed)name.replace(U".png",U"_inversion.png");return name;
}
struct TimingWordFeedback { double signedMs = 0; double displayedAt = 0; };
struct TimingFeedback {
    Judgment::JudgmentResult result = Judgment::JudgmentResult::kUnspecified;
    double signedMs = 0;
    double displayedAt = 0;
    Optional<TimingWordFeedback> retainedWord;
};
// One candidate per frame; equal magnitudes retain the existing lane processing order.
class TimingFeedbackState {
    Optional<TimingFeedback> m_pending, m_latest;
    Optional<TimingWordFeedback> m_word;
public:
    void offer(Judgment::JudgmentResult result, Optional<double> ms) {
        if (!ms || !std::isfinite(*ms)) return; // Auto and missed notes have no measured input.
        if (!m_pending || std::abs(*ms) > std::abs(m_pending->signedMs))
            m_pending = TimingFeedback{result, *ms, 0};
    }
    Optional<TimingFeedback> commit(double now) {
        if (m_pending) {
            m_latest=m_pending; m_latest->displayedAt=now;
            if (m_pending->result!=Judgment::JudgmentResult::kCritical && m_pending->signedMs!=0)
                m_word=TimingWordFeedback{m_pending->signedMs,now};
            m_pending.reset();
        }
        if (m_latest) m_latest->retainedWord=m_word;
        return m_latest;
    }
    void clear() { m_pending.reset(); m_latest.reset(); m_word.reset(); }
};
inline bool IsTimingNear(Judgment::JudgmentResult result) {
    return result==Judgment::JudgmentResult::kNearFast || result==Judgment::JudgmentResult::kNearSlow;
}
struct TimingPresentation {
    String word, ms;
    Color color{178,178,178}, wordColor{178,178,178};
    double alpha=0, wordAlpha=0;
};
inline constexpr double kTimingDisplaySeconds = 1.2;
inline constexpr double kTimingFadeSeconds = 0.36;
inline double TimingOpacity(double age) {
    return age<0 || age>=kTimingDisplaySeconds ? 0 : 0.9*(age<=kTimingDisplaySeconds-kTimingFadeSeconds
        ? 1.0 : (kTimingDisplaySeconds-age)/kTimingFadeSeconds);
}
inline TimingPresentation PresentTiming(const TimingHudSettings& settings, const TimingFeedback& event, double now) {
    TimingPresentation view;
    if (!std::isfinite(event.signedMs)) return view;
    view.alpha=TimingOpacity(now-event.displayedAt);
    const bool isNear=IsTimingNear(event.result);
    const double magnitude=std::abs(event.signedMs);
    const bool nearRange=magnitude>=Judgment::TimingWindow::ChipNote::kWindowSecCritical*1000.0;
    const bool critRange=magnitude>Judgment::kPerfectCriticalWindowMs;
    Optional<TimingWordFeedback> word=event.retainedWord;
    if (!word && (isNear || event.result==Judgment::JudgmentResult::kError) && event.signedMs!=0)
        word=TimingWordFeedback{event.signedMs,event.displayedAt};
    if (word) {
        view.wordAlpha=TimingOpacity(now-word->displayedAt);
        if (view.wordAlpha>0) view.word=word->signedMs>0 ? U"FAST" : U"SLOW";
        view.wordColor=JudgeDirectionColor(settings.judgeColor,word->signedMs>0);
    }
    if (view.alpha>0 && (settings.display==TimingMsDisplay::kAll || (settings.display==TimingMsDisplay::kNear && nearRange)
        || (settings.display==TimingMsDisplay::kCrit && critRange))) {
        view.ms=std::round(event.signedMs*10)==0 ? U"0.0ms" : U"{:+.1f}ms"_fmt(event.signedMs);
    }
    const bool colored=(settings.color==TimingMsColor::kNear && nearRange)
        || (settings.color==TimingMsColor::kCrit && critRange)
        || (settings.color==TimingMsColor::kSixteenMs && magnitude>16.0);
    if (colored && event.signedMs!=0) view.color=JudgeDirectionColor(settings.judgeColor,event.signedMs>0);
    return view;
}
inline double TimingPositionY(TimingPosition position) {
    constexpr std::array<double,4> y{78,169,299,411};
    return y[Clamp(static_cast<int32>(position),0,3)];
}
// Shared by navigation and tests, so disabled settings cannot receive a cursor.
inline int32 TimingMenuCursor(int32 cursor, int32 delta, bool newMode) {
    return Clamp(cursor,newMode?0:4,8);
}
}
