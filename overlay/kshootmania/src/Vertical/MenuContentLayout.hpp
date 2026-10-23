#pragma once
#include "VerticalLayout.hpp"
namespace Vertical::MenuContent {
// The portrait menu artwork and song-selection frame use this 720x1280 content window.
inline RectF Rect(){
 if(Scene::Height()<=Scene::Width())return RectF{Scene::Size()};
 const auto view=Viewport::Fit(Scene::Width(),Scene::Height());
 return {view.x(0),view.y(110),720*view.scale,1060*view.scale};
}
inline double Scale(){return Viewport::Fit(Scene::Width(),Scene::Height()).scale;}
}
