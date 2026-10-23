#pragma once
#include <Siv3D.hpp>
namespace MusicGame {
struct GaugeVisual {
    bool enabled = false;
    double percent = 0;
    double dodge = 0;
    double bannerOpacity = 0;
    double burn = 0;
};
// All values here are display-only. No scoring or result code reads this state.
class GaugePresentation {
    bool m_initialized = false, m_transition = false;
    double m_start = 0, m_elapsed = 0, m_previousChartTime = 0;
    GaugeVisual m_visual;
public:
    static constexpr double kPlaybackSpeed = 1.5;
    static constexpr double kDodgeHold = .7;
    static constexpr double kDodgeDuration = .7;
    static constexpr double kFadeDuration = .6;
    static constexpr double kBurnStart = .4;
    static constexpr double kBurnDuration = .4;
    static constexpr double kDodgeReleaseStart = .8;
    static constexpr double kDodgeReleaseDuration = .4;
    static double FadeStart() { return .5; }
    static double EaseOut(double t) { t=Clamp(t,0.0,1.0);return 1-(1-t)*(1-t); }
    static Optional<double>& FixtureStep() { static thread_local Optional<double> step; return step; }
    static Optional<double>& FixtureChartTime() { static thread_local Optional<double> time; return time; }
    static double FrameDelta() { return FixtureStep().value_or(Scene::DeltaTime()); }
    void reset() { *this=GaugePresentation{}; }
    const GaugeVisual& visual() const { return m_visual; }
    GaugeVisual update(double actual, bool protectedAps, bool switched, bool paused, double delta, double chartTime) {
        if(!m_initialized || chartTime < m_previousChartTime-.001) {
            m_initialized=true;m_transition=false;m_visual={true,actual,0,protectedAps?1.0:0.0};
        }
        m_previousChartTime=chartTime;
        if(switched) { m_start=m_visual.percent;m_elapsed=0;m_transition=true; }
        else if(m_transition && !paused) m_elapsed+=Max(0.0,delta)*kPlaybackSpeed;
        m_visual.enabled=true;
        if(m_transition) {
            const double blend=EaseOut(m_elapsed);
            m_visual.percent=m_start*(1-blend)+actual*blend;
            const double release=EaseOut((m_elapsed-kDodgeReleaseStart)/kDodgeReleaseDuration);
            m_visual.dodge=EaseOut(m_elapsed/kDodgeDuration)*(1-(1-kDodgeHold)*release);
            m_visual.burn=EaseOut((m_elapsed-kBurnStart)/kBurnDuration);
            const double fade=Clamp((m_elapsed-FadeStart())/kFadeDuration,0.0,1.0);
            m_visual.bannerOpacity=1-fade*fade;
            if(m_elapsed>=Max(FadeStart()+kFadeDuration,kDodgeReleaseStart+kDodgeReleaseDuration)-1e-9) { m_visual.percent=actual;m_visual.bannerOpacity=0;m_transition=false; }
        } else m_visual={true,actual,0,protectedAps?1.0:0.0};
        return m_visual;
    }
};
}
