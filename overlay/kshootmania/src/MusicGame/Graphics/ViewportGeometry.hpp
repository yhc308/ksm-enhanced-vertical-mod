#pragma once
#include "Common/SkinPreset.hpp"
#include "Graphics/GameRenderContext.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
namespace ViewportGeometry {
inline double LaserScale(){return SkinPreset::Current().laserWidthPercent/100.0;}
inline double NoteScale(){return SkinPreset::Current().noteHeightPercent/100.0;}
inline double CoreLeft(){return GameRender::VerticalEnabled()?32.0:44.0;}
inline double CoreWidth(){return 256.0-2*CoreLeft();}
// Percent always uses the landscape width; portrait's authored width is the 8/11 detent.
inline double BaseLaserWidth(){return 48.0;}
inline double NativeLaserRatio(){return GameRender::VerticalEnabled()?SkinPreset::kVerticalLaserPercent/100.0:1.0;}
inline double NativeLaserWidth(){return BaseLaserWidth()*NativeLaserRatio();}
inline double RailScale(){return LaserScale()/NativeLaserRatio();}
inline double LaserWidth(){return BaseLaserWidth()*LaserScale();}
// Landscape: 168+40*r; portrait: 192+40*r. Portrait retains its wider button core.
// At r=8/11, portrait rails, span and laser width exactly match the original portrait layout.
inline double LaserSpan(){return CoreWidth()+(256.0-NativeLaserWidth()-CoreWidth())*RailScale();}
inline double LaserSpanRatio(){return LaserSpan()/208.0;}
inline double LaserValue(double value){return .5+(value-.5)*LaserSpanRatio();}
inline double LaserCenter(double value,int32 xScale=1){return (128.0+(value-.5)*LaserSpan())*xScale;}
inline double RailX(double x){const double left=CoreLeft(),right=256-left;return x<left?left+(x-left)*RailScale():x>right?right+(x-right)*RailScale():x;}
inline double ButtonScale(){return GameRender::VerticalEnabled()?192.0/168.0:1.0;}
inline double WorldX(double centeredLogicalX){return centeredLogicalX/256*MusicGame::Graphics::kHighwayPlaneSize.x/.9925;}
inline double ButtonWorldX(bool bt,int lane,double splitSource=0){
    const double center=44+(bt?42:84)*lane+(bt?40:82)/2.0+splitSource;
    return WorldX((center-128)*ButtonScale());
}
inline double LaserWorldX(double value,bool wide=false){return WorldX((value-.5)*LaserSpan()*(wide?2:1));}
inline double NoteTop(double top,double height){return top+height*.8*(1.0-NoteScale());}
}
