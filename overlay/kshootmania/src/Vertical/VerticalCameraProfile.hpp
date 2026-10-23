#pragma once
#include "VerticalModConfig.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"

namespace Vertical
{
    inline constexpr Float3 kJudgmentBarCenter{0.0f, 2.6f, -MusicGame::Graphics::kHighwayPlaneSize.y / 2 - 0.4f};
    inline constexpr Float2 kJudgmentBarSize{344.0f / 8, 26.0f / 8};

    inline Float3 NeutralFarAnchor()
    {
        return {0.0f, 0.0f, static_cast<float>(MusicGame::Graphics::kHighwayPlaneSize.y * kJudgeTextureT / 2)};
    }
    inline Float3 NeutralNoteAnchor()
    {
        using namespace MusicGame::Graphics;
        return {0.0f, 0.0f, -kHighwayPlaneSize.y / 2 +
            kHighwayPlaneSize.y * kJdglineYFromBottom / kHighwayTextureSize.y};
    }

    struct CameraSolution
    {
        double fov=0, pitch=0, vanishingY=0;
        Vec3 eye{};
    };

    // Fit actual far edge, ray-plane judgment anchor and rail width together.
    // The authored zoom/tilt/spin transforms remain upstream and unchanged.
    inline CameraSolution SolveCamera(Size output, LayoutSettings layout)
    {
        layout.sanitize();
        const double top=output.y*layout.laneTopY, judge=output.y*layout.judgmentLineY;
        const double railHalf=MusicGame::Graphics::kHighwayPlaneSize.x/2/kHighwayUVScaleX;
        CameraSolution result;
        // Default gap is exactly 48 logical pixels. Extreme developer layouts
        // may need a deeper horizon to admit a finite perspective solution;
        // their requested visible top, anchor and width still remain exact.
        double gap=output.y*kVanishingPointGap;
        for(int attempt=0;attempt<16;++attempt,gap*=1.25) {
            const double horizon=top-gap;
            const double slope=output.x*layout.laneWidth/(2*(judge-horizon));
            const auto evaluate=[&](double fov) {
                result.fov=fov; result.vanishingY=horizon;
                const double focal=output.y/(2*Tan(fov/2));
                result.pitch=Atan((output.y*.5-horizon)/focal);
                const double c=Cos(result.pitch), s=Sin(result.pitch);
                const double height=railHalf*c/slope;
                const double z=NeutralFarAnchor().z+height*Tan(result.pitch)-focal*height/(gap*c*c);
                result.eye={0,height,z};
                const double depth=(height-kJudgmentBarCenter.y)*s+(kJudgmentBarCenter.z-z)*c;
                const double vertical=(kJudgmentBarCenter.y-height)*c+(kJudgmentBarCenter.z-z)*s;
                return depth>0 ? output.y*.5-focal*vertical/depth-judge : std::numeric_limits<double>::quiet_NaN();
            };
            double low=5_deg, high=low;
            double lower=evaluate(low);
            bool bracket=false;
            for(int degrees=6;degrees<=150;++degrees) {
                high=Math::ToRadians(static_cast<double>(degrees));
                const double upper=evaluate(high);
                if(std::isfinite(lower)&&std::isfinite(upper)&&lower*upper<=0) {bracket=true;break;}
                low=high;lower=upper;
            }
            if(!bracket) continue;
            for(int i=0;i<64;++i) {
                const double mid=(low+high)*.5;
                if(evaluate(mid)*lower>0) low=mid; else high=mid;
            }
            evaluate((low+high)*.5);
            return result;
        }
        // Sanitized supported layouts have a solution; keep invalid external
        // viewport inputs from propagating NaNs to the renderer.
        return {84.38635517454156_deg,18.775356343378913_deg,output.y*400.0/1280,{0,40.4997586483345,-350.7062130122202}};
    }

    inline BasicCamera3D MakeCamera(Size output, LayoutSettings layout)
    {
        layout.sanitize();
        struct Cache {Size size;LayoutSettings layout;CameraSolution solution;};
        static thread_local std::vector<Cache> cache;
        auto found=std::find_if(cache.begin(),cache.end(),[&](const Cache& item){return item.size==output && item.layout.laneWidth==layout.laneWidth && item.layout.judgmentLineY==layout.judgmentLineY && item.layout.laneTopY==layout.laneTopY;});
        if(found==cache.end()){
            if(cache.size()>=32)cache.erase(cache.begin());
            cache.push_back({output,layout,SolveCamera(output,layout)});found=std::prev(cache.end());
        }
        const auto& value=found->solution;
        return BasicCamera3D{output,value.fov,value.eye,value.eye+Vec3{0,-Sin(value.pitch),Cos(value.pitch)}};
    }

    inline double JudgmentBarWidthScale(const BasicCamera3D& camera, int outputWidth)
    {
        const float half = kJudgmentBarSize.x / 2;
        const double width = Abs(camera.worldToScreenPoint(kJudgmentBarCenter + Float3{half, 0, 0}).x -
            camera.worldToScreenPoint(kJudgmentBarCenter - Float3{half, 0, 0}).x);
        // Match orange core (source pixels 28..316 of cropped 334px), not silver caps.
        return outputWidth*kOrangeCoreWidthRatio/(289.0/334)/Max(width,1.0);
    }
}
