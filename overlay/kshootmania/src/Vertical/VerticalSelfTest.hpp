#pragma once
#include "VerticalCameraProfile.hpp"
#include "VerticalLaneGeometry.hpp"
#include "VerticalTunnelWarp.hpp"
#include "ProjectiveWall.hpp"

namespace Vertical
{
    inline void RunSelfTest()
    {
        TextWriter report{U"vertical-self-test.csv"};
        report.writeln(U"width,height,lane_width,judgment_y,lane_top_y,y_error_px,width_error_px,top_error_px,status");
        bool passed = true;
        for (const auto resolution : kPresets)
        {
            for (const double width : {0.70, kDefaultLaneWidth, 0.96})
            {
                for (const double lineY : {0.65, kDefaultJudgmentLineY, 0.90})
                {
                  for (const double topY : {0.20, 0.35, 0.40})
                  {
                    LayoutSettings settings{width, lineY, topY};
                    const auto camera = MakeCamera({resolution.width, resolution.height}, settings);
                    const auto eye=camera.getEyePosition();
                    const Vec3 bar{kJudgmentBarCenter};
                    const auto anchor=eye+(bar-eye)*(-eye.y/(bar.y-eye.y));
                    const float halfWidth = static_cast<float>(MusicGame::Graphics::kHighwayPlaneSize.x / 2 / kHighwayUVScaleX);
                    const double yError = Abs(camera.worldToScreenPoint(anchor).y - resolution.height * lineY);
                    const double actualWidth = Abs(camera.worldToScreenPoint(anchor + Float3{halfWidth, 0, 0}).x -
                        camera.worldToScreenPoint(anchor - Float3{halfWidth, 0, 0}).x);
                    const double widthError = Abs(actualWidth - resolution.width * width);
                    const float farZ = NeutralFarAnchor().z;
                    const double topError = Abs(camera.worldToScreenPoint(Float3{0, 0, farZ}).y - resolution.height * topY);
                    const bool ok = std::isfinite(yError) && std::isfinite(widthError) &&
                        std::isfinite(topError) && topError <= resolution.height / 1280.0 &&
                        yError <= resolution.height / 1280.0 && widthError <= resolution.width / 720.0;
                    passed = passed && ok;
                    report.writeln(U"{},{},{},{},{},{},{},{},{}"_fmt(resolution.width, resolution.height,
                        width, lineY, topY, yError, widthError, topError, ok ? U"PASS" : U"FAIL"));
                  }
                }
            }
        }
        TextWriter guideReport{U"vertical-guide-calibration.csv"};
        guideReport.writeln(U"width,height,fov_deg,pitch_deg,vanishing_x,vanishing_y,top_y,top_width,judge_y,judge_width,button_width,rail_width,end_y,end_width,orange_core_width,max_error_px,status");
        for(const auto res:kPresets) {
            const Size size{res.width,res.height};
            const auto fit=SolveCamera(size,{});
            const auto camera=MakeCamera(size,{});
            const auto eye=camera.getEyePosition();const Vec3 bar{kJudgmentBarCenter};
            const auto anchor=eye+(bar-eye)*(-eye.y/(bar.y-eye.y));
            const auto span=NeutralLaneSpan(camera,res.height);
            const double farZ=NeutralFarAnchor().z;
            const double endZ=farZ+(-MusicGame::Graphics::kHighwayPlaneSize.y/2-farZ)*span.endT;
            const float half=static_cast<float>(MusicGame::Graphics::kHighwayPlaneSize.x/2/kHighwayUVScaleX);
            const double scale=res.height/1280.0;
            const auto widthAt=[&](double z){return Abs(camera.worldToScreenPoint(Float3{half,0,static_cast<float>(z)}).x-camera.worldToScreenPoint(Float3{-half,0,static_cast<float>(z)}).x)/scale;};
            const double top=camera.worldToScreenPoint(NeutralFarAnchor()).y/scale;
            const double judge=camera.worldToScreenPoint(kJudgmentBarCenter).y/scale;
            const double end=camera.worldToScreenPoint(Float3{0,0,static_cast<float>(endZ)}).y/scale;
            const double topWidth=widthAt(farZ),judgeWidth=widthAt(anchor.z),endWidth=widthAt(endZ);
            const double slope=.5508076027535542;
            const double error=Max({Abs(fit.vanishingY/scale-400),Abs(top-448),Abs(judge-906),Abs(end-934),
                Abs(topWidth-2*slope*48),Abs(judgeWidth-2*slope*506),Abs(endWidth-2*slope*534)});
            const bool ok=std::isfinite(error)&&error<=1;passed=passed&&ok;
            guideReport.writeln(U"{},{},{},{},360,{},{},{},{},{},{},{},{},{},{},{},{}"_fmt(res.width,res.height,ToDegrees(fit.fov),ToDegrees(fit.pitch),fit.vanishingY/scale,top,topWidth,judge,judgeWidth,judgeWidth*kButtonWidthRatio,judgeWidth*kLaserRailWidthRatio,end,endWidth,kOrangeCoreWidthRatio*720,error,ok?U"PASS":U"FAIL"));
        }
        // Exercise the actual shared gear transform at all supported resolutions.
        // Includes authored full rotations; do not clamp them to the sample range.
        const bool savedEnabled = ConfigIni::GetBool(U"vertical_enabled", false);
        ConfigIni::SetBool(U"vertical_enabled", true);
        TextWriter gearReport{U"vertical-gear-origin-test.csv"};
        gearReport.writeln(U"width,height,degrees,far_x,far_y,bar_x,bar_y,rigid_error,status");
        for (const auto resolution : kPresets) {
            const auto camera = MakeCamera({resolution.width,resolution.height},{});
            const auto farPoint = NeutralFarAnchor();
            const auto nearPoint = NeutralNoteAnchor();
            for (double degrees : {-360.,-180.,-90.,-25.,-17.5,-10.,-7.5,-5.,-2.5,0.,2.5,5.,7.5,10.,17.5,25.,90.,180.,360.}) {
                const auto matrix = MusicGame::Graphics::GearTiltTransformMatrix(Math::ToRadians(degrees));
                const auto fp = camera.worldToScreenPoint(matrix.transformPoint(farPoint));
                const auto bp = camera.worldToScreenPoint(matrix.transformPoint(kJudgmentBarCenter));
                const double rigidError = Abs((matrix.transformPoint(farPoint)-matrix.transformPoint(nearPoint)).length()-(farPoint-nearPoint).length());
                bool ok = std::isfinite(fp.x) && std::isfinite(fp.y) && std::isfinite(bp.x) && std::isfinite(bp.y) && rigidError < 0.001;
                if (Abs(degrees)<=10) ok = ok && Abs(fp.x-resolution.width*.5)<=resolution.width*4.0/720 &&
                    Abs(fp.y-resolution.height*.35)<=resolution.height*4.0/1280;
                if (degrees==0) ok = ok && (matrix.transformPoint(nearPoint)-nearPoint).length()<0.001;
                passed = passed && ok;
                gearReport.writeln(U"{},{},{},{},{},{},{},{},{}"_fmt(resolution.width,resolution.height,degrees,fp.x,fp.y,bp.x,bp.y,rigidError,ok ? U"PASS" : U"FAIL"));
            }
        }
        TextWriter laneReport{U"vertical-lane-anchor-test.csv"};
        laneReport.writeln(U"width,height,degrees,anchor_error_px,tail_px,outline_error_px,button_ratio,side_ratio,status");
        bool lanePassed=true;
        for(const auto resolution:kPresets) {
            const auto camera=MakeCamera({resolution.width,resolution.height},{});
            auto source=MeshData::Grid(MusicGame::Graphics::kHighwayPlaneSizeWide,1,1);
            const float height=MusicGame::Graphics::kHighwayPlaneSize.y;
            source.vertices[0].pos.z=source.vertices[1].pos.z=static_cast<float>(height*kJudgeTextureT/2);
            source.vertices[2].pos.z=source.vertices[3].pos.z=-height/2;
            for(double degrees:{-10.,-7.5,-5.,-2.5,0.,2.5,5.,7.5,10.}) {
                MusicGame::ViewStatus view;
                view.tiltRadians=Math::ToRadians(degrees);
                const auto span=DisplayLaneSpan(source,camera,view,resolution.height);
                const auto display=DisplayHighwayMesh(source,span);
                const auto transform=HighwayOrigin(view);
                const auto ap=camera.worldToScreenPoint(transform.transformPoint((display.vertices[2].pos+display.vertices[3].pos)*.5f));
                const auto bp=camera.worldToScreenPoint(BarWorldCenter(view));
                const auto ep=camera.worldToScreenPoint(transform.transformPoint((display.vertices[4].pos+display.vertices[5].pos)*.5f));
                const double error=(ap-bp).length()*720/resolution.width;
                const double tail=(ep.y-bp.y)*1280/resolution.height;
                const auto fp=camera.worldToScreenPoint(transform.transformPoint(source.vertices[0].pos));
                const auto np=camera.worldToScreenPoint(transform.transformPoint(source.vertices[2].pos));
                const auto dp=camera.worldToScreenPoint(transform.transformPoint(display.vertices[4].pos));
                const double outline=Abs((dp.x-fp.x)*(np.y-fp.y)-(dp.y-fp.y)*(np.x-fp.x))/Max((np-fp).length(),1.0f)*720/resolution.width;
                const double buttonRatio=168.0*kButtonWidthScale/256;
                const double sideRatio=44.0*kLaserWidthScale/256;
                const bool ok=span.rayValid && error<=4 && outline<=1 &&
                    (degrees!=0 || Abs(tail-kTailLogicalPixels)<=1) && Abs(buttonRatio-kButtonWidthRatio)<1e-6 && Abs(sideRatio-kLaserRailWidthRatio)<1e-6;
                lanePassed=lanePassed && ok;
                laneReport.writeln(U"{},{},{},{},{},{},{},{},{}"_fmt(resolution.width,resolution.height,degrees,error,tail,outline,buttonRatio,sideRatio,ok ? U"PASS" : U"FAIL"));
            }
        }
        // An authored edge-on surface must remain finite when no ray intersection exists.
        auto degenerate=MeshData::Grid(MusicGame::Graphics::kHighwayPlaneSizeWide,1,1);
        degenerate.vertices[2].pos=degenerate.vertices[0].pos;
        degenerate.vertices[3].pos=degenerate.vertices[1].pos;
        const auto fallback=DisplayLaneSpan(degenerate,MakeCamera({720,1280},{}),{},1280);
        const bool fallbackOk=!fallback.rayValid && std::isfinite(fallback.anchorT) && std::isfinite(fallback.endT);
        lanePassed=lanePassed && fallbackOk;
        laneReport.writeln(U"edge_on_fallback,,,,,,,,{}"_fmt(fallbackOk ? U"PASS" : U"FAIL"));
        passed=passed && lanePassed;
        laneReport.writeln(U"overall,,,,,,,,{}"_fmt(lanePassed ? U"PASS" : U"FAIL"));
        ConfigIni::SetBool(U"vertical_enabled", false);
        const auto legacy = MusicGame::Graphics::TiltTransformMatrix(0.3);
        const auto off = MusicGame::Graphics::GearTiltTransformMatrix(0.3);
        const bool legacyOk = (legacy.transformPoint(NeutralFarAnchor())-off.transformPoint(NeutralFarAnchor())).length()<0.001;
        passed = passed && legacyOk;
        gearReport.writeln(U"legacy_off,,,,,,,,{}"_fmt(legacyOk ? U"PASS" : U"FAIL"));
        ConfigIni::SetBool(U"vertical_enabled", savedEnabled);
        LayoutSettings invalid{NAN, INFINITY, -INFINITY};
        TextWriter layoutReport{U"vertical-ingame-layout-test.csv"};
        layoutReport.writeln(U"width,height,bar_width_ratio,bar_y_ratio,far_width_ratio,status");
        for (const auto resolution : kPresets)
        {
            const auto camera = MakeCamera({resolution.width, resolution.height}, {});
            const float half = static_cast<float>(MusicGame::Graphics::kHighwayPlaneSize.x / 2 / kHighwayUVScaleX);
            const double topWidth = Abs(camera.worldToScreenPoint(NeutralFarAnchor() + Float3{half, 0, 0}).x -
                camera.worldToScreenPoint(NeutralFarAnchor() - Float3{half, 0, 0}).x) / resolution.width;
            const float barHalf = static_cast<float>(kJudgmentBarSize.x * JudgmentBarWidthScale(camera, resolution.width) / 2);
            const double barWidth = Abs(camera.worldToScreenPoint(kJudgmentBarCenter + Float3{barHalf, 0, 0}).x -
                camera.worldToScreenPoint(kJudgmentBarCenter - Float3{barHalf, 0, 0}).x) / resolution.width;
            const double barY = camera.worldToScreenPoint(kJudgmentBarCenter).y / resolution.height;
            const auto span=NeutralLaneSpan(camera,resolution.height);
            const float anchorZ=static_cast<float>(MusicGame::Graphics::kHighwayPlaneSize.y*kJudgeTextureT/2-
                MusicGame::Graphics::kHighwayPlaneSize.y*(.5+kJudgeTextureT/2)*span.anchorT);
            const float visibleHalf=half;
            const double gearWidth=Abs(camera.worldToScreenPoint(Float3{visibleHalf,0,anchorZ}).x-
                camera.worldToScreenPoint(Float3{-visibleHalf,0,anchorZ}).x)/resolution.width;
            const bool ok = Abs(barWidth*(289.0/334)-kOrangeCoreWidthRatio) <= 1.0 / resolution.width && Abs(gearWidth-kDefaultLaneWidth)<=1.0/resolution.width &&
                Abs(topWidth-52.87753/720)<=1.0/resolution.width && Abs(barY-kDefaultJudgmentLineY)<=1.0/resolution.height;
            passed = passed && ok;
            layoutReport.writeln(U"{},{},{},{},{},{}"_fmt(resolution.width, resolution.height, barWidth, barY, topWidth, ok ? U"PASS" : U"FAIL"));
        }
        invalid.sanitize();
        const bool invalidOk = invalid.laneWidth == kDefaultLaneWidth && invalid.judgmentLineY == kDefaultJudgmentLineY && invalid.laneTopY == kDefaultLaneTopY;
        passed = passed && invalidOk;
        report.writeln(U"sanitization,,,,,,,,{}"_fmt(invalidOk ? U"PASS" : U"FAIL"));
        ConfigIni::Load();
        const Array<String> protectedKeys{U"globaloffset", U"inputdelay", U"laserdelay", U"visual_offset",
            U"soundfx_delay", U"vsync", U"key", U"key2", U"joy", U"joy2", U"bt_mode", U"fx_mode", U"laser_mode",
            U"fullscr", U"windowsize", U"fullscrsize", U"autosync", U"audio_backend", U"mastervol"};
        Array<String> before;
        for (const auto& key : protectedKeys) before.push_back(String{ConfigIni::GetString(key)});
        const auto original = ModConfig::Load();
        auto changed = original;
        changed.enabled = !original.enabled;
        changed.backgroundDepthEnabled=false;
        changed.windowSize = {1080, 1920};
        changed.layout = {0.72, 0.86, 0.25};
        changed.Save();
        const auto reloaded = ModConfig::Load();
        bool configOk = !reloaded.backgroundDepthEnabled && reloaded.enabled == changed.enabled && reloaded.windowSize.width == 1080 &&
            reloaded.layout.laneWidth == 0.72 && reloaded.layout.judgmentLineY == 0.86;
        changed.depthIntensities={0,200,120,80,12,99,15,45};changed.Save();
        configOk=configOk && ModConfig::Load().depthIntensities==changed.depthIntensities;
        changed.ResetDepthImage(5);configOk=configOk && changed.depthIntensities[5]==35 && changed.depthIntensities[1]==200;
        changed.ResetDepthImages();configOk=configOk && changed.depthIntensities==kDepthImageDefaults && !changed.backgroundDepthEnabled;
        changed.depthIntensities[0]=-1;changed.depthIntensities[1]=201;changed.Save();
        configOk=configOk && ModConfig::Load().depthIntensities[0]==0 && ModConfig::Load().depthIntensities[1]==200;
        configOk=configOk && ParseDepthPercent(U"0")==Optional<int>{0} && ParseDepthPercent(U"200")==Optional<int>{200}
            && !ParseDepthPercent(U"201") && !ParseDepthPercent(U"-1") && !ParseDepthPercent(U"1.5") && !ParseDepthPercent(U"") && !ParseDepthPercent(U"abc");
        changed.ResetLayout();
        changed.Save();
        configOk = configOk && !ModConfig::Load().backgroundDepthEnabled && ModConfig::Load().layout.judgmentLineY == kDefaultJudgmentLineY;
        for (size_t i = 0; i < protectedKeys.size(); ++i)
            configOk = configOk && before[i] == ConfigIni::GetString(protectedKeys[i]);
        original.Save();
        const int originalVersion = ConfigIni::GetInt(U"vertical_layout_version", 0);
        ConfigIni::SetInt(U"vertical_layout_version", 0);
        ConfigIni::SetDouble(U"vertical_lane_width", 0.95);
        ConfigIni::SetDouble(U"vertical_judgment_y", 0.89);
        bool migrationOk = ModConfig::MigrateLayout() && Abs(ModConfig::Load().layout.laneWidth-kDefaultLaneWidth)<1e-10 &&
            ModConfig::Load().layout.judgmentLineY == kDefaultJudgmentLineY && ModConfig::Load().layout.laneTopY == kDefaultLaneTopY;
        ConfigIni::SetDouble(U"vertical_lane_width", 0.81);
        migrationOk = migrationOk && !ModConfig::MigrateLayout() && ModConfig::Load().layout.laneWidth == 0.81;
        for (size_t i = 0; i < protectedKeys.size(); ++i) migrationOk = migrationOk && before[i] == ConfigIni::GetString(protectedKeys[i]);
        original.Save();
        ConfigIni::SetInt(U"vertical_layout_version", originalVersion);
        passed = passed && migrationOk;
        report.writeln(U"layout_migration_once_preservation,,,,,,,,{}"_fmt(migrationOk ? U"PASS" : U"FAIL"));
        passed = passed && configOk;
        report.writeln(U"config_roundtrip_reset_preservation,,,,,,,,{}"_fmt(configOk ? U"PASS" : U"FAIL"));
        report.writeln(U"overall,,,,,,,,{}"_fmt(passed ? U"PASS" : U"FAIL"));
        TextWriter tunnelReport{U"vertical-tunnel-test.csv"};
        tunnelReport.writeln(U"width,height,angle,requested,applied,valid,min_area_ratio,projection_error,status");
        bool tunnelPassed=true;
        for(const auto res:kPresets) {
            const auto camera=MakeCamera({res.width,res.height},{});
            const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();
            for(auto& vertex:source.vertices) vertex.pos=camera.screenToWorldPoint(
                Float2{vertex.tex.x*res.width,static_cast<float>((240+576*vertex.tex.y)*res.height/1280)},depth);
            TunnelWarp warp{source,true,true};
            const auto reference=camera.screenToWorldPoint(Float2{res.width*.5f,res.height*.35f},depth);
            for(double angle:{-90.,-10.,0.,10.,90.}) for(double movement:{-400.,-80.,-40.,0.,40.,80.,400.}) {
                const auto transform=Mat4x4::Rotate(camera.getLookAtVector(),Math::ToRadians(angle),reference);
                const auto target=camera.screenToWorldPoint(Float2{res.width*.5f,static_cast<float>(res.height*(.35+movement/1280))},depth);
                warp.update(camera,transform,transform.transformPoint(target),transform.transformPoint(reference));
                const double error=warp.projectionError(camera,transform)*1280/res.height;
                const bool ok=std::isfinite(warp.delta()) && error<.01 &&
                    (!warp.valid() || warp.minAreaRatio()>=.1999) &&
                    (angle!=0 || !(-40<=movement&&movement<=40) || (warp.valid() && Abs(warp.delta()-movement*res.height/1280)<.01));
                tunnelPassed=tunnelPassed&&ok;
                tunnelReport.writeln(U"{},{},{},{},{},{},{},{},{}"_fmt(res.width,res.height,angle,movement,warp.delta()*1280/res.height,warp.valid(),warp.minAreaRatio(),error,ok?U"PASS":U"FAIL"));
            }
        }
        TextWriter projectiveReport{U"vertical-projective-wall-test.csv"};
        projectiveReport.writeln(U"width,height,upper_delta,lower_delta,lower_scale,valid,projective,middle_move,shape_error,boundary_move,projection_error,min_area,status");
        bool projectivePassed=true;
        for(const auto res:kPresets) {
            const auto camera=MakeCamera({res.width,res.height},{});
            const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();
            for(auto& vertex:source.vertices) vertex.pos=camera.screenToWorldPoint(Float2{vertex.tex.x*res.width,static_cast<float>((240+576*vertex.tex.y)*res.height/1280)},depth);
            TunnelWarp warp{source,true,true};
            TunnelWarp::GearQuad reference{Float3{res.width*.46f,res.height*.35f,depth},Float3{res.width*.54f,res.height*.35f,depth},
                Float3{res.width*.085f,res.height*.70475f,depth},Float3{res.width*.915f,res.height*.70475f,depth}};
            for(double upper:{-100.,-50.,0.,50.,100.}) for(double lower:{-80.,0.,80.}) for(double scale:{.6,1.,1.4}) {
                auto actual=reference;
                for(int i=0;i<4;++i) {
                    actual[i].y+=static_cast<float>((i<2 ? upper : lower)*res.height/1280);
                    if(i>=2) actual[i].x=static_cast<float>(res.width*.5+(actual[i].x-res.width*.5)*scale);
                }
                warp.updateGear(camera,Mat4x4::Identity(),actual,reference);
                const double error=warp.projectionError(camera,Mat4x4::Identity())*1280/res.height;
                const bool neutral=upper==0 && lower==0 && scale==1;
                const bool moving=upper!=0 || lower!=0 || scale!=1;
                const bool ok=warp.valid() && warp.projective() && error<.02 && warp.protectedShapeError()<1e-6 &&
                    warp.uvError()<1e-7 && warp.straightError()<.001 && warp.minAreaRatio()>=.1999 &&
                    (!neutral || warp.middleMove()<.001) && (!moving || warp.middleMove()>.001);
                projectivePassed=projectivePassed&&ok;
                projectiveReport.writeln(U"{},{},{},{},{},{},{},{},{},{},{},{},{}"_fmt(res.width,res.height,upper,lower,scale,warp.valid(),warp.projective(),warp.middleMove(),warp.protectedShapeError(),warp.boundaryMove(),error,warp.minAreaRatio(),ok?U"PASS":U"FAIL"));
            }
            auto singular=reference;for(auto& v:singular) v.x=res.width*.5f;
            warp.updateGear(camera,Mat4x4::Identity(),singular,reference);
            const bool singularFallback=warp.valid() && !warp.projective() && warp.minAreaRatio()>=.1999;
            projectivePassed=projectivePassed&&singularFallback;
            projectiveReport.writeln(U"singular_fallback,,,,,,,,,,,,{}"_fmt(singularFallback?U"PASS":U"FAIL"));
        }
        TextWriter thinReport{U"vertical-wall-thickness-test.csv"};
        thinReport.writeln(U"width,height,angle,lower_ratio,target_thickness,applied_thickness,uv_error,straight_error,area,status");
        bool thinPassed=true;
        for(const auto res:kPresets) {
            const auto camera=MakeCamera({res.width,res.height},{});
            const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();
            for(auto& vertex:source.vertices) vertex.pos=camera.screenToWorldPoint(Float2{static_cast<float>((-144+1008*vertex.tex.x)*res.width/720),static_cast<float>((180.2+806.4*vertex.tex.y)*res.height/1280)},depth);
            TunnelWarp warp{source,true,true};
            TunnelWarp::GearQuad reference{Float3{res.width*.46f,res.height*.35f,depth},Float3{res.width*.54f,res.height*.35f,depth},Float3{res.width*.085f,res.height*.70475f,depth},Float3{res.width*.915f,res.height*.70475f,depth}};
            warp.updateGear(camera,Mat4x4::Identity(),reference,reference);
            const auto pivot=camera.screenToWorldPoint(Float2{res.width*.5f,res.height*.35f},depth);
            for(double angle:{-10.,0.,10.}) for(double scale:{1.,.9,.8,.6,.4}) {
                const auto transform=Mat4x4::Rotate(camera.getLookAtVector(),Math::ToRadians(angle),pivot);
                auto actual=reference;
                for(int i=2;i<4;++i) actual[i].x=static_cast<float>(res.width*.5+(actual[i].x-res.width*.5)*scale);
                warp.updateGear(camera,transform,actual,reference);
                const bool ok=warp.valid() && warp.uvError()<1e-7 && warp.straightError()<.001 && warp.protectedShapeError()<1e-6 && warp.minAreaRatio()>=.1999 &&
                    Abs(warp.thickness()-Clamp(scale,.6,1.0))<1e-5 && (angle!=0 || Abs(warp.sideThicknessRatio()-Clamp(scale,.6,1.0))<.001);
                thinPassed=thinPassed&&ok;
                thinReport.writeln(U"{},{},{},{},{},{},{},{},{},{}"_fmt(res.width,res.height,angle,scale,warp.thickness(),warp.sideThicknessRatio(),warp.uvError(),warp.straightError(),warp.minAreaRatio(),ok?U"PASS":U"FAIL"));
            }
        }
        thinReport.writeln(U"overall,,,,,,,,,{}"_fmt(thinPassed?U"PASS":U"FAIL"));
        projectivePassed=projectivePassed&&thinPassed;
        projectiveReport.writeln(U"overall,,,,,,,,,,,,{}"_fmt(projectivePassed?U"PASS":U"FAIL"));
        tunnelReport.writeln(U"overall,,,,,,,,{}"_fmt(tunnelPassed?U"PASS":U"FAIL"));
        TextWriter panelReport{U"vertical-projective-panel-test.csv"};
        panelReport.writeln(U"width,height,angle,top,lower,scale,valid,line_error,uv_error,seam_error,core_error,area,cover_margin,status");
        bool panelPassed=true;
        for(const auto res:kPresets) {
            const auto camera=MakeCamera({res.width,res.height},{});
            const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();
            for(auto& v:source.vertices)v.pos=camera.screenToWorldPoint(Float2{static_cast<float>((-2+724*v.tex.x)*res.width/720),static_cast<float>((239.169444+579.2*v.tex.y)*res.height/1280)},depth);
            ProjectiveWall wall{source,true,true};wall.setProfile(U"imgs/bg/arrow.gif",kDepthImageDefaults);
            ProjectiveWall::GearQuad reference{Float3{res.width*.46f,res.height*.35f,depth},Float3{res.width*.54f,res.height*.35f,depth},Float3{res.width*.085f,res.height*.70475f,depth},Float3{res.width*.915f,res.height*.70475f,depth}};
            wall.updateGear(camera,Mat4x4::Identity(),reference,reference);
            const auto pivot=camera.screenToWorldPoint(Float2{res.width*.5f,res.height*.35f},depth);
            for(double angle:{-10.,0.,10.})for(double top:{-100.,0.,100.})for(double lower:{-80.,0.,80.})for(double scale:{.6,1.,1.4}) {
                const auto transform=Mat4x4::Rotate(camera.getLookAtVector(),Math::ToRadians(angle),pivot);
                auto actual=reference;
                for(int i=0;i<4;++i){actual[i].y+=static_cast<float>((i<2?top:lower)*res.height/1280);if(i>=2)actual[i].x=static_cast<float>(res.width*.5+(actual[i].x-res.width*.5)*scale);}
                wall.updateGear(camera,transform,actual,reference);
                const bool ok=wall.valid()&&wall.lineError()*1280/res.height<.01&&wall.uvError()<1e-7&&wall.sharedEdgeError()*1280/res.height<.01&&wall.protectedShapeError()*1280/res.height<.01&&wall.minAreaRatio()>=.1999&&wall.projectionError(camera,transform)*1280/res.height<.02&&wall.margin()*720/res.width>=1.99;
                panelPassed=panelPassed&&ok;
                panelReport.writeln(U"{},{},{},{},{},{},{},{},{},{},{},{},{},{}"_fmt(res.width,res.height,angle,top,lower,scale,wall.valid(),wall.lineError(),wall.uvError(),wall.sharedEdgeError(),wall.protectedShapeError(),wall.minAreaRatio(),wall.margin(),ok?U"PASS":U"FAIL"));
            }
        }
        // Profile selection must not classify custom files by their basename.
        for(const auto file:{U"imgs/bg/sakura.gif",U"imgs/bg/snow.gif",U"custom/arrow.gif",U"imgs/bg/hidden.gif"}) {
            const auto camera=MakeCamera({720,1280},{});const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();for(auto& v:source.vertices)v.pos=camera.screenToWorldPoint(Float2{-2+724*v.tex.x,239.169444f+579.2f*v.tex.y},depth);
            ProjectiveWall wall{source,true,true};wall.setProfile(file,kDepthImageDefaults);
            ProjectiveWall::GearQuad ref{Float3{330,448,depth},Float3{390,448,depth},Float3{60,900,depth},Float3{660,900,depth}};auto actual=ref;actual[2].x=180;actual[3].x=540;
            wall.updateGear(camera,Mat4x4::Identity(),actual,ref);
            const double expected=StringView{file}==U"custom/arrow.gif"?.8875:.8425;
            const bool ok=StringView{file}==U"imgs/bg/hidden.gif"?!wall.valid():wall.valid()&&Abs(wall.thickness()-expected)<1e-6;
            panelPassed=panelPassed&&ok;panelReport.writeln(U"profile,{},,,,,,,,,,,,{}"_fmt(file,ok?U"PASS":U"FAIL"));
        }
        {
            const auto camera=MakeCamera({720,1280},{});const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();for(auto& v:source.vertices)v.pos=camera.screenToWorldPoint(Float2{-2+724*v.tex.x,239.169444f+579.2f*v.tex.y},depth);
            ProjectiveWall wall{source,true,true};ProjectiveWall::GearQuad ref{Float3{330,448,depth},Float3{390,448,depth},Float3{60,900,depth},Float3{660,900,depth}};
            for(const float bad:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
                auto actual=ref;actual[0].x=bad;wall.updateGear(camera,Mat4x4::Identity(),actual,ref);
                const bool ok=!wall.valid();panelPassed=panelPassed&&ok;
                panelReport.writeln(U"invalid_coordinate,,,,,,,,,,,,,{}"_fmt(ok?U"PASS":U"FAIL"));
            }
        }
        TextWriter strengthReport{U"vertical-depth-strength-test.csv"};
        strengthReport.writeln(U"image,strength,lower,valid,thickness,uv_error,shared_edge_error,status");
        bool strengthPassed=true;
        for(size_t image=0;image<kDepthImageNames.size();++image) for(int strength:{0,200}) for(double lower:{-1.0,0.0,1.0}) {
            const auto camera=MakeCamera({720,1280},{});const float depth=camera.worldToScreenPoint(Float3{0,-41,0}).z;
            auto source=MeshData::Billboard();for(auto& v:source.vertices)v.pos=camera.screenToWorldPoint(Float2{-2+724*v.tex.x,239.169444f+579.2f*v.tex.y},depth);
            auto strengths=kDepthImageDefaults;strengths[image]=strength;
            ProjectiveWall wall{source,true,true};wall.setProfile(image==7 ? U"custom/test.gif" : U"imgs/bg/{}.gif"_fmt(kDepthImageNames[image]),strengths);
            ProjectiveWall::GearQuad ref{Float3{330,448,depth},Float3{390,448,depth},Float3{60,900,depth},Float3{660,900,depth}};auto actual=ref;
            actual[2].x=360-300*(1+.4*lower);actual[3].x=360+300*(1+.4*lower);
            wall.updateGear(camera,Mat4x4::Identity(),actual,ref,lower);
            const bool ok=strength==0 ? !wall.valid() : wall.valid() && wall.thickness()>=.25 && wall.uvError()<1e-6 && wall.sharedEdgeError()<.001;
            strengthPassed=strengthPassed&&ok;
            strengthReport.writeln(U"{},{},{},{},{},{},{},{}"_fmt(kDepthImageNames[image],strength,lower,wall.valid(),wall.thickness(),wall.uvError(),wall.sharedEdgeError(),ok?U"PASS":U"FAIL"));
        }
        strengthReport.writeln(U"overall,,,,,,,{}"_fmt(strengthPassed?U"PASS":U"FAIL"));
        panelPassed=panelPassed&&strengthPassed;
        panelReport.writeln(U"overall,,,,,,,,,,,,,{}"_fmt(panelPassed?U"PASS":U"FAIL"));
        // A projection test is not a gameplay/input/latency regression test.
    }
}
