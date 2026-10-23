#pragma once
#include "MusicGame/Graphics/Highway/Highway3DGraphics.hpp"
#include "MusicGame/Graphics/Jdgline/LaserCursor3DGraphics.hpp"
#include "MusicGame/Graphics/Jdgline/Jdgoverlay3DGraphics.hpp"
#include "Vertical/VerticalLaneGeometry.hpp"
#include "MusicGame/Graphics/Jdgline/Jdgline3DGraphics.hpp"
namespace RenderGeometryFixture {
inline bool Requested(){return System::GetCommandLineArgs().contains(U"--render-geometry-test");}
inline Rect Bounds(const Image& image){
 int l=image.width(),r=-1,t=image.height(),b=-1;
 for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){const auto c=image[y][x];if(c.g>100){l=Min(l,x);r=Max(r,x);t=Min(t,y);b=Max(b,y);}}
 return r<l?Rect{0,0,0,0}:Rect{l,t,r-l+1,b-t+1};
}
inline void Run(){
 if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"render-geometry-fixture")))throw Error{U"Render tests require an isolated fixture"};
 using namespace MusicGame;using namespace MusicGame::Graphics;
 TextWriter report{FsUtils::GetResourcePath(U"render-geometry.csv")};report.writeln(U"check,actual,expected,status");int count=0;
 const auto check=[&](StringView name,double actual,double expected,double tolerance){const bool ok=std::isfinite(actual)&&Abs(actual-expected)<=tolerance;report.writeln(U"{},{},{},{}"_fmt(name,actual,expected,ok?U"PASS":U"FAIL"));++count;if(!ok)throw Error{U"Render geometry failed: {} actual={} expected={}"_fmt(name,actual,expected)};};
 // These independent contracts catch both the wrong plane/texture ratio and a wide-laser center shift.
 check(U"texture_pixels_per_lane",kHighwayTextureSizeWide.x/(kHighwayPlaneSizeWide.x/kHighwayPlaneSize.x),256,.001);
 check(U"wide_and_normal_share_center",kLaserPositionOffsetWide.x+256,kHighwayTextureWideCenterX,.001);
 const bool vertical=GameRender::VerticalEnabled();const auto original=SkinPreset::Current();
 const Array<Size> sizes=vertical?Array<Size>{{720,1280},{1080,1920}}:Array<Size>{{640,480},{1280,720}};
 for(const auto size:sizes){
  const GameRender::Scope scope{GameRender::Context{size,vertical}};
  BasicCamera3D camera=vertical?Vertical::MakeCamera(size,Vertical::ModConfig::Load().layout):BasicCamera3D{size,45_deg,Vec3{0,45,-366},Vec3{0,45-100*Sin(15_deg),-366+100*Cos(15_deg)}};
  Graphics3D::SetCameraTransform(camera);
  Highway3DGraphics highway;KeyBeamGraphics beams;ButtonNoteGraphics notes;LaserCursor3DGraphics cursor;Jdgoverlay3DGraphics overlay{camera};
  HighwayRenderTexture laneTarget;RenderTexture output{size,HasDepth::Yes};
  Jdgline3DGraphics baselineBar;
  {const ScopedRenderTarget3D target{output.clear(Palette::Black)};baselineBar.draw3D(ViewStatus{});Graphics3D::Flush();}
  Image baselineBarImage;output.readAsImage(baselineBarImage);const auto baselineBarBounds=Bounds(baselineBarImage);
  for(const double percent:{50.0,SkinPreset::kVerticalLaserPercent,100.0,125.0}){
   auto settings=original;settings.laserWidthPercent=percent;SkinPreset::Activate(settings);
   check(U"mode_specific_laser_width",ViewportGeometry::LaserWidth(),48*percent/100,1e-6);
   check(U"mode_specific_laser_span",ViewportGeometry::LaserSpan(),(vertical?192:168)+40*percent/100,1e-6);
   ViewStatus view;highway.update(view);const auto anchor=highway.displayNoteAnchor();
   double referenceCursorWidth=0;
   for(const bool wide:{false,true})for(const double value:{.25,.5,.75}){
    GameStatus game;game.laserLaneStatus[0].cursorX=value;game.laserLaneStatus[0].cursorWide=wide;
    {const ScopedRenderTarget3D target{output.clear(Palette::Black)};cursor.draw3D(game,view,camera,anchor);Graphics3D::Flush();}
    Image image;output.readAsImage(image);const auto box=Bounds(image);
    // Independently project the texture coordinate through the actual padded mesh.
    const double tx=(wide?kLaserPositionOffsetWide.x:kLaserPositionOffsetNormal.x)+ViewportGeometry::LaserCenter(value,wide?2:1);
    const double u=tx/kHighwayTextureSizeWide.x;
    const double worldX=(u-.5)*kHighwayPlaneSizeWide.x/.9925;
    const double originalX=(ViewportGeometry::LaserValue(value)-.5)*282/8*(wide?2:1)*Camera::JdgoverlayScale(0);
    const auto expected=camera.worldToScreenPoint(vertical?anchor+Float3{static_cast<float>(worldX),0,0}:Float3{static_cast<float>(originalX),3.5f,-kHighwayPlaneSize.y/2-1.8f});
    check(U"cursor_center_x",box.center().x,expected.x,2.1);check(U"cursor_center_y",box.center().y,expected.y,2.1);
    if(!wide&&value==.5){referenceCursorWidth=box.w;image.save(FsUtils::GetResourcePath(U"cursor-{}-{}.png"_fmt(size.x,static_cast<int>(percent))));}
    check(U"cursor_visible",box.w>0?1:0,1,0);
   }
   check(U"cursor_width_mode",referenceCursorWidth,(vertical?80.0*size.y/1280/(8.0/11):camera.worldToScreenPoint(Float3{2.25f,3.5f,-kHighwayPlaneSize.y/2-1.8f}).x-camera.worldToScreenPoint(Float3{-2.25f,3.5f,-kHighwayPlaneSize.y/2-1.8f}).x)*percent/100,2.5);
  }
  for(const double split:{0.0,100.0,-100.0})for(int lane=0;lane<6;++lane){
   const bool bt=lane<4;const int index=bt?lane:lane-4;const int number=bt?4:2;ViewStatus view;view.camStatus.centerSplit=split;
   GameStatus game;game.currentTimeSec=.075;auto& status=bt?game.btLaneStatus[index]:game.fxLaneStatus[index];status.keyBeamTimeSec=0;
   laneTarget.additiveTexture().clear(Palette::Black);beams.draw(game,view,laneTarget);Graphics2D::Flush();Image beam;laneTarget.additiveTexture().readAsImage(beam);const auto b=Bounds(beam);
   kson::ChartData chart;chart.beat.bpm.emplace(0,120);chart.beat.timeSig.emplace(0,kson::TimeSig{});
   (bt?chart.note.bt[index]:chart.note.fx[index]).emplace(0,kson::Interval{});const auto cache=kson::CreateTimingCache(chart.beat);Scroll::HighwayScroll scroll{chart};scroll.update(HispeedSetting{},120);Scroll::HighwayScrollContext context{&scroll,&chart.beat,&cache,&game};
   for(const int height:{50,100,200}){
    auto settings=original;settings.noteHeightPercent=height;SkinPreset::Activate(settings);laneTarget.additiveTexture().clear(Palette::Black);notes.draw(chart,game,view,PlayOption{},context,laneTarget);Graphics2D::Flush();Image note;laneTarget.additiveTexture().readAsImage(note);const auto n=Bounds(note);
    check(U"zero_time_note_keeps_original_scroll_row",context.getPositionY(0),1010,0);
    const double shift=Camera::CenterSplitShiftX(split)*(index>=number/2?1:-1);
    const double expected=kHighwayTextureWideCenterX+((44+(bt?42:84)*index+(bt?40:82)/2.0+shift)-128)*(vertical?192.0/168:1);
    check(U"note_center",n.center().x,expected,1.5);check(U"beam_matches_note",b.center().x,n.center().x,1.1);check(U"note_width",n.w,(bt?40:82)*(vertical?192.0/168:1),2.1);
   }
   highway.update(view);const auto anchor=highway.displayNoteAnchor();status.chipAnim.push({game.currentTimeSec-.1,Judgment::ChipAnimType::kCritical});
   overlay.draw2D(game,view);Graphics2D::Flush();
   {const ScopedRenderTarget3D target{output.clear(Palette::Black)};overlay.draw3D(view,camera,anchor);Graphics3D::Flush();}
   Image effect;output.readAsImage(effect);const auto e=Bounds(effect);
   const double shift=Camera::CenterSplitShiftX(split)*(index>=number/2?1:-1);
   const double logical=((44+(bt?42:84)*index+(bt?40:82)/2.0+shift)-128)*(vertical?192.0/168:1);
   const double originalCenter=904.0/4+92+(bt?0:30)+(bt?60:120)*index+static_cast<int32>(Camera::CenterSplitShiftX(split,60)*(index>=number/2?1:-1))+43;
   const double originalX=(originalCenter-452)*.95/8*Camera::JdgoverlayScale(0);
   const auto expected=camera.worldToScreenPoint(vertical?anchor+Float3{static_cast<float>(logical/256*kHighwayPlaneSize.x/.9925),0,0}:Float3{static_cast<float>(originalX),3.5f,-kHighwayPlaneSize.y/2-1.8f});
   check(U"chip_effect_matches_note",e.center().x,expected.x,2.5);
   check(U"chip_ring_matches_rendered_bar_y",e.center().y,baselineBarBounds.center().y,2.5);
   // New assets use exactly the same lane geometry and overlay plane as CRITICAL.
   status.keyBeamType=Judgment::KeyBeamType::kPerfectCritical;
   laneTarget.additiveTexture().clear(Palette::Black);beams.draw(game,view,laneTarget);Graphics2D::Flush();
   Image perfectBeam;laneTarget.additiveTexture().readAsImage(perfectBeam);const auto pb=Bounds(perfectBeam);
   check(U"perfect_beam_matches_critical_center",pb.center().x,b.center().x,1.1);
   check(U"perfect_beam_matches_critical_width",pb.w,b.w,1.1);
   status.chipAnim={};status.chipAnim.push({game.currentTimeSec-.1,Judgment::ChipAnimType::kPerfectCritical});
   overlay.draw2D(game,view);Graphics2D::Flush();
   {const ScopedRenderTarget3D target{output.clear(Palette::Black)};overlay.draw3D(view,camera,anchor);Graphics3D::Flush();}
   Image perfectEffect;output.readAsImage(perfectEffect);const auto pe=Bounds(perfectEffect);
   check(U"perfect_effect_matches_critical_center",pe.center().x,e.center().x,1.1);
   check(U"perfect_effect_matches_critical_width",pe.w,e.w,1.1);
  }
  // Camera zoom must change cursor and laser widths together, independently of portrait text sizing.
  for(const double percent:{50.0,100.0,125.0}){
   auto settings=original;settings.laserWidthPercent=percent;SkinPreset::Activate(settings);
   double baselineWidth=0,baselineNoteWidth=0;
   for(const double zoom:{0.0,-100.0,100.0}){
    ViewStatus view;view.camStatus.zoomBottom=zoom;highway.update(view);const auto anchor=highway.displayNoteAnchor();
    GameStatus game;game.laserLaneStatus[0].cursorX=.5;
    {const ScopedRenderTarget3D target{output.clear(Palette::Black)};cursor.draw3D(game,view,camera,anchor);Graphics3D::Flush();}
    Image image;output.readAsImage(image);const auto box=Bounds(image);
    const double half=ViewportGeometry::LaserWidth()/256*kHighwayPlaneSize.x/.9925/2;
    const double noteWidth=(camera.worldToScreenPoint(anchor+Float3{static_cast<float>(half),0,0})-camera.worldToScreenPoint(anchor-Float3{static_cast<float>(half),0,0})).length();
    if(zoom==0){baselineWidth=box.w;baselineNoteWidth=noteWidth;}
    check(U"cursor_zoom_tracks_note_width",box.w,baselineWidth*(vertical?noteWidth/baselineNoteWidth:Camera::JdglineScale(zoom)/Camera::JdglineScale(0)),3);
    const auto expected=camera.worldToScreenPoint(vertical?anchor:Float3{0,3.5f,-kHighwayPlaneSize.y/2-1.8f});check(U"cursor_zoom_hit_center_x",box.center().x,expected.x,2.1);check(U"cursor_zoom_hit_center_y",box.center().y,expected.y,2.1);
   }
  }
  for(const bool legacy:{false,true})for(const double zoom:{0.0,-100.0,100.0}) {
   ViewStatus pose;pose.camStatus.zoomBottom=zoom;pose.camStatus.useLegacyJdgScale=legacy;pose.camStatus.useLegacyZoomTop=legacy;
   highway.update(pose);const auto hit=highway.displayNoteAnchor();Jdgline3DGraphics bar;
   {const ScopedRenderTarget3D target{output.clear(Palette::Black)};bar.draw3D(pose);Graphics3D::Flush();}
   Image barImage;output.readAsImage(barImage);const auto barBounds=Bounds(barImage);
   GameStatus event;event.laserLaneStatus[0].cursorX=.5;
   {const ScopedRenderTarget3D target{output.clear(Palette::Black)};cursor.draw3D(event,pose,camera,hit);Graphics3D::Flush();}
   Image cursorImage;output.readAsImage(cursorImage);const auto cursorBounds=Bounds(cursorImage);
   check(U"cursor_matches_rendered_bar_y",cursorBounds.center().y,barBounds.center().y,2.1);
   check(U"cursor_matches_rendered_bar_x",cursorBounds.center().x,barBounds.center().x,2.1);
   check(U"bar_visible",barBounds.w>0?1:0,1,0);
   if(zoom==0 && !legacy){
    const ScopedRenderTarget3D target{output.clear(Palette::Black)};highway.draw3D(pose);bar.draw3D(pose);cursor.draw3D(event,pose,camera,hit);Graphics3D::Flush();
    Image combined;output.readAsImage(combined);combined.save(FsUtils::GetResourcePath(U"bar-cursor-aligned-{}.png"_fmt(size.x)));
   }
  }
  // A foreground depth-writing surface must not bury judgment feedback rendered afterwards.
  ViewStatus view;highway.update(view);const auto anchor=vertical?highway.displayNoteAnchor():Float3{0,3.5f,-kHighwayPlaneSize.y/2-1.8f};GameStatus game;game.laserLaneStatus[0].cursorX=.5;
  const Mesh foreground{MeshData::Billboard()};const Vec3 closer=Vec3{anchor}+(camera.getEyePosition()-Vec3{anchor})*.01;
  {
   const ScopedRenderTarget3D target{output.clear(Palette::Black)};
   {const ScopedRenderStates3D depth{DepthStencilState::DepthTestWrite};foreground.draw(camera.billboard(closer,Float2{200,200}),Palette::Red);}
   cursor.draw3D(game,view,camera,anchor);Graphics3D::Flush();
  }
  Image layered;output.readAsImage(layered);const auto center=camera.worldToScreenPoint(anchor);check(U"cursor_draws_above_depth_surface",layered[static_cast<int>(center.y)][static_cast<int>(center.x)].g,255,1);
  layered.save(FsUtils::GetResourcePath(U"cursor-layer-{}.png"_fmt(size.x)));
  game.btLaneStatus[1].chipAnim.push({-.1,Judgment::ChipAnimType::kCritical});overlay.draw2D(game,view);Graphics2D::Flush();
  {const ScopedRenderTarget3D target{output.clear(Palette::Black)};{const ScopedRenderStates3D depth{DepthStencilState::DepthTestWrite};foreground.draw(camera.billboard(closer,Float2{200,200}),Palette::Red);}overlay.draw3D(view,camera,anchor);Graphics3D::Flush();}
  output.readAsImage(layered);check(U"judgment_effect_draws_above_depth_surface",Bounds(layered).w>0?1:0,1,0);
  game.btLaneStatus[1].chipAnim={};game.btLaneStatus[1].chipAnim.push({-.1,Judgment::ChipAnimType::kPerfectCritical});
  overlay.draw2D(game,view);Graphics2D::Flush();
  {const ScopedRenderTarget3D target{output.clear(Palette::Black)};{const ScopedRenderStates3D depth{DepthStencilState::DepthTestWrite};foreground.draw(camera.billboard(closer,Float2{200,200}),Palette::Red);}overlay.draw3D(view,camera,anchor);Graphics3D::Flush();}
  output.readAsImage(layered);check(U"perfect_effect_draws_above_depth_surface",Bounds(layered).w>0?1:0,1,0);

 }
 SkinPreset::Activate(original);report.writeln(U"overall,,,PASS");TextWriter{FsUtils::GetResourcePath(U"render-geometry-passed.txt")}.writeln(U"TOTAL {} checks PASS"_fmt(count));
}
}

