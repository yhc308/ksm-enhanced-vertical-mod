#pragma once
#include "Graphics/GameRenderContext.hpp"
#include "VerticalCameraProfile.hpp"
#include "MusicGame/Graphics/ViewportGeometry.hpp"
#include "MusicGame/ViewStatus.hpp"
#include "MusicGame/Camera/CameraMath.hpp"

namespace Vertical
{
    inline constexpr double kButtonWidthScale = kButtonWidthRatio / (168.0/256);
    inline constexpr double kLaserWidthScale = kLaserRailWidthRatio / (44.0/256);
    inline double LaserWidth() { return ViewportGeometry::LaserWidth(); }
    inline double LaserWorldX(double value, bool wide=false) {return ViewportGeometry::LaserWorldX(value,wide);}
    inline double ButtonWorldX(bool bt,int lane,double splitSource=0) {return ViewportGeometry::ButtonWorldX(bt,lane,splitSource);}
    inline Mat4x4 HighwayOrigin(const MusicGame::ViewStatus& view) {
        return MusicGame::Camera::CamShiftXMatrix(view.camStatus.shiftX)*
            MusicGame::Graphics::GearTiltTransformMatrix(Math::ToRadians(view.camStatus.rotationDeg+view.camStatus.rotationDegHighway)+view.tiltRadians);
    }
    inline MusicGame::ViewStatus SharedGearView(MusicGame::ViewStatus view) {
        view.camStatus.rotationDegHighway=0;view.camStatus.rotationDegJdgline=0;
        return view;
    }
    inline bool OriginalSpinFitProbe() {
        static const bool probe=System::GetCommandLineArgs().contains(U"--vertical-preview-hud") && System::GetCommandLineArgs().contains(U"--vertical-preview-old-spin-fit");
        return probe;
    }
    inline Mat4x4 HUDGearOrigin(const MusicGame::ViewStatus& view) {
        // Approach warnings keep shared tilt/translation, not the
        // independent slam-event spin which can carry the 3D anchor offscreen.
        // Combo placement is a fixed viewport HUD coordinate in GraphicsMain.
        return HighwayOrigin(OriginalSpinFitProbe() ? view : SharedGearView(view));
    }
    inline Float3 BarWorldCenter(const MusicGame::ViewStatus& view) {
        const auto& cam=view.camStatus;
        const double scale=cam.useLegacyJdgScale ? MusicGame::Camera::LegacyJdgoverlayScale(cam.zoomBottom) : MusicGame::Camera::JdgoverlayScale(cam.zoomBottom);
        const auto transform=MusicGame::Graphics::GearTiltTransformMatrix(Math::ToRadians(cam.rotationDeg+cam.rotationDegJdgline)+view.tiltRadians);
        return transform.transformPoint(kJudgmentBarCenter+Float3{static_cast<float>(cam.shiftX*scale),0,0});
    }
    struct LaneSpan { double anchorT; double endT; bool rayValid=true; };
    inline LaneSpan NeutralLaneSpan(const BasicCamera3D& camera,int outputHeight=GameRender::Height()) {
        const auto eye=camera.getEyePosition();
        const Vec3 bar{kJudgmentBarCenter};
        const Vec3 anchor=eye+(bar-eye)*(-eye.y/(bar.y-eye.y));
        const double h=MusicGame::Graphics::kHighwayPlaneSize.y;
        const double farZ=h*kJudgeTextureT/2;
        const double frontZ=-h/2;
        double lo=eye.z+.1,hi=anchor.z;
        const double target=camera.worldToScreenPoint(kJudgmentBarCenter).y+outputHeight*kTailLogicalPixels/1280;
        for(int i=0;i<48;++i) {
            const double z=(lo+hi)/2;
            if(camera.worldToScreenPoint(Float3{0,0,static_cast<float>(z)}).y>target) lo=z; else hi=z;
        }
        return {(anchor.z-farZ)/(frontZ-farZ),((lo+hi)/2-farZ)/(frontZ-farZ)};
    }
    // Intersect the bar's camera ray with the original (possibly zoomed/flipped)
    // highway plane. Only its texture-to-distance mapping changes. The far edge
    // and the projected outer sidelines above the bar remain the same lines.
    // Edge-on authored poses use the neutral span rather than divide by zero.
    inline LaneSpan DisplayLaneSpan(const MeshData& source,const BasicCamera3D& camera,const MusicGame::ViewStatus& view,int outputHeight=GameRender::Height()) {
        auto result=NeutralLaneSpan(camera,outputHeight);
        // Slam-event highway and judgment-line spins intentionally differ upstream.
        // Fit the neutral distance in their shared frame, then rotate the whole
        // gear once. Intersecting independently rotated planes crosses a pole.
        const auto fittingView=OriginalSpinFitProbe() ? view : SharedGearView(view);
        const auto inverse=HighwayOrigin(fittingView).inverse();
        const auto eye=inverse.transformPoint(Float3{camera.getEyePosition()});
        const auto bar=inverse.transformPoint(BarWorldCenter(fittingView));
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
    // Add a vertex row at the texture's existing judgment row. Scroll position,
    // note lifetime, negative scroll and judgment APIs keep the original values.
    inline MeshData DisplayHighwayMesh(const MeshData& source,const LaneSpan& span) {
        auto mesh=MeshData::Grid(MusicGame::Graphics::kHighwayPlaneSizeWide,1,2);
        const std::array<double,3> lengths{0,span.anchorT,span.endT};
        const std::array<float,3> uv{0,static_cast<float>(kJudgeTextureT),1};
        for(int row=0;row<3;++row) for(int col=0;col<2;++col) {
            auto& v=mesh.vertices[row*2+col];
            v.pos=source.vertices[col].pos+(source.vertices[col+2].pos-source.vertices[col].pos)*static_cast<float>(lengths[row]);
            v.tex={source.vertices[col].tex.x,uv[row]};
        }
        return mesh;
    }
}
