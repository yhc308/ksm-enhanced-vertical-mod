#pragma once
#include <NocoUI.hpp>
#include "Vertical/VerticalResources.hpp"
#include "Common/FsUtils.hpp"
#include "MusicGame/TimingFeedback.hpp"
#include "TimingTextStyle.hpp"

namespace MusicGame::Graphics {
class TimingOverlay {
    std::shared_ptr<noco::Canvas> m_canvas, m_glowCanvas;
public:
    TimingOverlay() {
        const auto path=Vertical::UIPath(U"ui/scene/timing_feedback.noco");
        m_canvas=noco::Canvas::LoadFromFile(path);
        m_glowCanvas=noco::Canvas::LoadFromFile(path);
        if (!m_canvas || !m_glowCanvas) throw Error{U"Failed to load '{}'"_fmt(path)};
    }
    void update(FastSlowMode mode, const TimingHudSettings& settings,
        const Optional<TimingFeedback>& feedback, double now, bool enabled=true) {
        const auto view=feedback && enabled && mode==FastSlowMode::kNew
            ? PresentTiming(settings,*feedback,now) : TimingPresentation{};
        const auto updateCanvas=[&](const std::shared_ptr<noco::Canvas>& canvas, bool glow) {
            canvas->setParamValues({{U"timingWord",view.word},{U"timingMs",view.ms}});
            const auto field=[&](StringView prefix, Color color, double opacity) {
                ApplyTimingTextStyle(*canvas,prefix,color,opacity,glow);
            };
            field(U"word",view.wordColor,view.wordAlpha);
            field(U"ms",view.color,view.alpha);
            for(int32 i=0;i<4;++i) canvas->setParamValue(U"timingVisible{}"_fmt(i),
                (view.alpha>0 || view.wordAlpha>0) && i==static_cast<int32>(settings.position));
            canvas->update(GameRender::Size(),noco::HitTestEnabledYN::No);
        };
        updateCanvas(m_canvas,false);updateCanvas(m_glowCanvas,true);
    }
    void draw() const {
        m_canvas->draw();
        // Same-color additive/dodge-like pass brightens glyphs without a moving halo.
        const ScopedRenderStates2D additive{BlendState::Additive};
        m_glowCanvas->draw();
    }
};
}
