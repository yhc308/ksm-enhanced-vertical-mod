#pragma once
#include "VerticalLaneGeometry.hpp"
#include "MusicGame/Camera/CamPattern/CamPatternMain.hpp"
#include "kson/kson.hpp"
namespace Vertical {
    inline LaneSpan OriginalSpinFit(const MeshData& source,const BasicCamera3D& camera,const MusicGame::ViewStatus& view,int outputHeight=Scene::Height()) {
        auto result=NeutralLaneSpan(camera,outputHeight);
        const auto inverse=HighwayOrigin(view).inverse();
        const auto eye=inverse.transformPoint(Float3{camera.getEyePosition()});
        const auto bar=inverse.transformPoint(BarWorldCenter(view));
        const auto f=source.vertices[0].pos;
        const auto n=source.vertices[2].pos;
        const double ry=bar.y-eye.y,rz=bar.z-eye.z;
        const double dy=n.y-f.y,dz=n.z-f.z;
        const double determinant=ry*dz-rz*dy;
        if(Abs(determinant)<1e-5) {result.rayValid=false;return result;}
        const double t=((f.y-eye.y)*rz-(f.z-eye.z)*ry)/determinant;
        if(!std::isfinite(t) || Abs(t)>10000) {result.rayValid=false;return result;}
        result.endT=t+(result.endT-result.anchorT);
        result.anchorT=t;
        return result;
    }
    inline void RunSpinRegression() {
        ConfigIni::Load();const auto original=ModConfig::Load();auto settings=original;settings.enabled=true;settings.Save();
        FilePath path=U"../../test-output/spin-v13/Tornaido Test.kson";
        const auto args=System::GetCommandLineArgs();for(size_t i=0;i+1<args.size();++i)if(args[i]==U"--vertical-preview-chart")path=args[i+1];
        const auto chart=kson::LoadKsonChartData(path.toUTF8());
        TextWriter report{U"vertical-spin-regression.csv"};report.writeln(U"width,height,event,pulse,highway_degrees,bar_degrees,anchor_t,local_anchor_error,clip_w,screen_step,old_anchor_t,hud_anchor_error,status");
        bool passed=!chart.camera.cam.pattern.laser.slamEvent.spin.empty();int count=0,oldBad=0;
        for(const auto res:kPresets) {
            const auto camera=MakeCamera({res.width,res.height},{});
            auto source=MeshData::Grid(MusicGame::Graphics::kHighwayPlaneSizeWide,1,1);
            const float h=MusicGame::Graphics::kHighwayPlaneSize.y;
            source.vertices[0].pos.z=source.vertices[1].pos.z=h*static_cast<float>(kJudgeTextureT)/2;
            source.vertices[2].pos.z=source.vertices[3].pos.z=-h/2;
            const auto neutral=DisplayLaneSpan(source,camera,{},res.height);
            const auto fixed=DisplayHighwayMesh(source,neutral);
            const auto localAnchor=(fixed.vertices[2].pos+fixed.vertices[3].pos)*.5f;
            for(const auto& [pulse,event]:chart.camera.cam.pattern.laser.slamEvent.spin) {
                MusicGame::Camera::CamPatternMain pattern{chart};pattern.onLaserSlamJudged(pulse,event.d,pulse);
                Vec2 previous{};bool hadPrevious=false;
                for(kson::Pulse elapsed=0;elapsed<=event.length*3/2;++elapsed) {
                    MusicGame::ViewStatus view;pattern.applyToCamStatus(view.camStatus,pulse+elapsed);
                    const auto span=DisplayLaneSpan(source,camera,view,res.height);const auto mesh=DisplayHighwayMesh(source,span);
                    const auto anchor=(mesh.vertices[2].pos+mesh.vertices[3].pos)*.5f;
                    const double error=(anchor-localAnchor).length();const auto world=HighwayOrigin(view).transformPoint(anchor);
                    DirectX::XMFLOAT4 p{world.x,world.y,world.z,1},clip;
                    DirectX::XMStoreFloat4(&clip,DirectX::XMVector4Transform(DirectX::XMLoadFloat4(&p),camera.getViewProj().value));
                    const auto screen=camera.worldToScreenPoint(world);const Vec2 point{screen.xy()};
                    const double step=hadPrevious ? (point-previous).length()*1280/res.height : 0;
                    const auto old=OriginalSpinFit(source,camera,view,res.height);
                    if(!old.rayValid || old.anchorT<.85 || old.anchorT>1.10)++oldBad;
                    const auto hud=camera.worldToScreenPoint(HUDGearOrigin(view).transformPoint(anchor));
                    const double hudError=(hud-camera.worldToScreenPoint(localAnchor)).length();
                    const bool ok=hudError<.001 && span.rayValid && error<.001 && clip.w>0 && std::isfinite(point.x)&&std::isfinite(point.y)&&step<25;
                    passed=passed&&ok;++count;previous=point;hadPrevious=true;
                    report.writeln(U"{},{},{},{},{},{},{},{},{},{},{},{},{}"_fmt(res.width,res.height,pulse,elapsed,view.camStatus.rotationDegHighway,view.camStatus.rotationDegJdgline,span.anchorT,error,clip.w,step,old.anchorT,hudError,ok?U"PASS":U"FAIL"));
                }
            }
        }
        report.writeln(U"overall,,,,,,,,,,,,{}"_fmt(passed?U"PASS":U"FAIL"));
        TextWriter summary{U"vertical-spin-summary.txt"};summary.writeln(U"conditions={} old_unstable_conditions={} result={}"_fmt(count,oldBad,passed?U"PASS":U"FAIL"));
        original.Save();
    }
}
