#pragma once
#include "TimingTextStyle.hpp"
#include "Graphics/NumberTextureFont.hpp"
#include "Common/SkinPreset.hpp"
#include "Common/FsUtils.hpp"
#include "Common/AssetManagement.hpp"
#include "MusicGame/ViewStatus.hpp"
#include "Vertical/VerticalHUDLayout.hpp"

namespace MusicGame::Graphics {
class PlayJudgeIndicator {
    Texture m_panel, m_miniPanel;
    Optional<JudgeColor> m_palette;
    std::shared_ptr<noco::Canvas> m_canvas, m_glow, m_miniCanvas, m_miniGlow;
    bool m_bottomVisible=false, m_miniVisible=false;
    static void Load(std::shared_ptr<noco::Canvas>& normal,std::shared_ptr<noco::Canvas>& glow,StringView name) {
        const auto path=FsUtils::GetResourcePath(U"mods/vertical/ui/"+String{name}+U".noco");
        normal=noco::Canvas::LoadFromFile(path);glow=noco::Canvas::LoadFromFile(path);
        if(!normal || !glow)throw Error{U"Failed to load play judge indicator: "+String{name}};
    }
    static void DrawText(const std::shared_ptr<noco::Canvas>& normal,const std::shared_ptr<noco::Canvas>& glow) {
        normal->draw();const ScopedRenderStates2D additive{BlendState::Additive};glow->draw();
    }
    static void LayoutMiniAverage(noco::Canvas& canvas,const String& mean,const String& absolute) {
        const auto font=AssetManagement::SystemFontBold();
        const double meanUnits=font(mean).region(1.0).w,absoluteUnits=font(absolute).region(1.0).w;
        // Keep the MSDF outline/glow inside the black band, clear of the Avg. label.
        constexpr double left=42.0,right=122.0,gap=1.5,padding=1.5;
        const double size=Min(9.5,(right-left-gap-2*padding)/Max(1.0,meanUnits+absoluteUnits));
        const double meanWidth=meanUnits*size+padding;
        const bool vertical=GameRender::VerticalEnabled();const double factor=vertical?1.05:1.0;
        const double origin=vertical?24.0:-300.0;
        for(bool second:{false,true}){
            const auto node=canvas.findByName(second?U"MiniAbsolute":U"MiniAverage");
            if(!node || !node->anchorRegion())throw Error{U"Mini average layout node missing"};
            auto region=*node->anchorRegion();
            region.posDelta.x=origin+(left+(second?meanWidth+gap:0))*factor;
            region.sizeDelta.x=(second?right-left-meanWidth-gap:meanWidth)*factor;
            node->setRegion(region);
            if(const auto label=node->getComponent<noco::Label>()){
                label->setFontSize(size*factor);label->setSizingMode(noco::LabelSizingMode::Fixed);
            }
        }
    }
public:
    // One local resource/canvas, positioned identically in preview and gameplay.
    static RectF MiniRect() {
        if(GameRender::VerticalEnabled()){
            const auto v=Vertical::Viewport::Fit(GameRender::Width(),GameRender::Height());
            return {v.x(24),v.y(318),140*1.05*v.scale,160*1.05*v.scale};
        }
        const double scale=GameRender::Height()/480.0;
        return {GameRender::Width()/2.0-300*scale,116*scale,140*scale,160*scale};
    }
    void update(const ViewStatus& view,JudgeColor palette=JudgeColor::kFastBlue,bool forceBoth=false) {
        const auto& settings=SkinPreset::Current();
        m_bottomVisible=GameRender::VerticalEnabled() && (forceBoth || settings.indicatorPortrait!=SkinPreset::IndicatorPortrait::Off);
        m_miniVisible=forceBoth || (GameRender::VerticalEnabled()?settings.indicatorPortrait==SkinPreset::IndicatorPortrait::Both:settings.indicatorLandscape);
        if(!m_bottomVisible && !m_miniVisible)return;
        if(m_bottomVisible && !m_canvas){m_panel=SkinPreset::LoadTexture(JudgeImage(U"play_judge_indicator.png",palette));Load(m_canvas,m_glow,U"play_judge_indicator");}
        if(m_miniVisible && !m_miniCanvas){m_miniPanel=SkinPreset::LoadTexture(JudgeImage(U"play_judge_indicator_minimum.png",palette));Load(m_miniCanvas,m_miniGlow,GameRender::VerticalEnabled()?U"play_judge_indicator_minimum_vertical":U"play_judge_indicator_minimum");}
        if(m_palette && *m_palette!=palette) {
            if(m_bottomVisible)m_panel=SkinPreset::LoadTexture(JudgeImage(U"play_judge_indicator.png",palette));
            if(m_miniVisible)m_miniPanel=SkinPreset::LoadTexture(JudgeImage(U"play_judge_indicator_minimum.png",palette));
        }
        m_palette=palette;
        const auto& d=view.detailedJudgments;
        const std::array<int32,7> counts{d.fastError,d.fastNear,d.fastCritical,d.perfectCritical,d.slowCritical,d.slowNear,d.slowError};
        // Both presentations read the original result's same unrounded population.
        const double mean=Round(view.inputTiming.meanMs()*10)/10;
        const String number=forceBoth&&!view.inputTiming.count?U"—":mean==0?U"0.0":U"{:+.1f}"_fmt(mean);
        const String absolute=forceBoth&&!view.inputTiming.count?U"—":U"±{:.1f}ms"_fmt(view.inputTiming.absoluteMeanMs());
        const Color color=JudgeTimingColor(palette,mean);
        for(bool mini:{false,true}){
            if(mini?!m_miniVisible:!m_bottomVisible)continue;
            for(bool glow:{false,true}){
                auto& canvas=*(mini?(glow?m_miniGlow:m_miniCanvas):(glow?m_glow:m_canvas));
                canvas.setParamValues({{U"countersVisible",!glow},{U"averageText",mini||(!view.inputTiming.count&&forceBoth)?number:number+U"ms"},{U"absoluteText",absolute}});
                for(size_t i=0;i<counts.size();++i)canvas.setParamValue(U"count{}"_fmt(i),Format(counts[i]));
                ApplyTimingTextStyle(canvas,U"average",color,.9,glow);
                ApplyTimingTextStyle(canvas,U"absolute",Color{178,178,178},.9,glow);
                if(mini)LayoutMiniAverage(canvas,number,absolute);
                canvas.update(GameRender::Size(),noco::HitTestEnabledYN::No);
            }
        }
    }
    void drawSettings(const RectF& bottom,const RectF& mini) const {
        // The gameplay canvases define every digit/average position, font, size and effect.
        // Only transform the complete indicator to this screen's requested HUD location.
        const auto drawAt=[&](const RectF& source,const RectF& target,const Texture& panel,const std::shared_ptr<noco::Canvas>& normal,const std::shared_ptr<noco::Canvas>& glow){
            if(target.w<=0 || !normal)return;
            panel.resized(target.size).draw(target.pos);
            const auto matrix=Mat3x2::Translate(-source.pos)*Mat3x2::Scale(target.w/source.w)*Mat3x2::Translate(target.pos);
            // SubCanvas resets its local transform, so the complete HUD move belongs to the camera transform.
            const Transformer2D transform{matrix,Transformer2D::Target::PushCamera};DrawText(normal,glow);
        };
        const auto v=Vertical::Viewport::Fit(GameRender::Width(),GameRender::Height());
        drawAt({v.x(0),v.y(963),720*v.scale,242*v.scale},bottom,m_panel,m_canvas,m_glow);
        drawAt(MiniRect(),mini,m_miniPanel,m_miniCanvas,m_miniGlow);
    }
    void draw() const {
        if(m_bottomVisible && m_canvas){
            const auto v=Vertical::Viewport::Fit(GameRender::Width(),GameRender::Height());
            m_panel.resized(720*v.scale,242*v.scale).draw(v.x(0),v.y(963));DrawText(m_canvas,m_glow);
        }
        if(m_miniVisible && m_miniCanvas){
            const auto rect=MiniRect();
            m_miniPanel.resized(rect.size).draw(rect.pos);DrawText(m_miniCanvas,m_miniGlow);
        }
    }
};
}
