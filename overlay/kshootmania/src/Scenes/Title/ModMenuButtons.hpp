#pragma once
#include "Input/InputUtils.hpp"
#include "UI/ModMenuStyle.hpp"
namespace ModMenuButtons {
inline double Unit(){return Min(Scene::Width()/640.0,Scene::Height()/720.0);}
inline RectF IconRect(int32 column){const bool portrait=Scene::Height()>Scene::Width();const double s=portrait?Scene::Width()/720.0:Unit();const double size=(portrait?64:56)*s,gap=12*s;return {Scene::Width()/2.0+(column-1)*(size+gap)-size/2,portrait?622*s:20*s,size,size};}
inline RectF JudgeRect(){return IconRect(0);}
inline RectF CustomRect(){return IconRect(2);}
inline RectF VerticalRect(){return IconRect(1);}
inline bool JudgeClicked(){return InputUtils::IsUIMouseInputEnabled()&&JudgeRect().leftClicked();}
inline int32 HoveredIndex(const Vec2& point){for(int32 i=0;i<3;++i)if(IconRect(i).contains(point))return i;return -1;}
inline void DrawJudge(bool focused){
 const auto rect=JudgeRect();ModMenuStyle::IconPanel(rect,focused);
 static const Texture icon{FsUtils::GetResourcePath(U"ui/menu_judge_icon.png")};
 icon.scaled(rect.w*.875/Max(icon.width(),icon.height())).drawAt(rect.center());
}
inline bool Clicked(bool vertical){return InputUtils::IsUIMouseInputEnabled()&&(vertical?VerticalRect():CustomRect()).leftClicked();}
inline void Draw(bool vertical,bool focused){
 const auto rect=vertical?VerticalRect():CustomRect();ModMenuStyle::IconPanel(rect,focused);
 static const Texture custom{FsUtils::GetResourcePath(U"ui/menu_custom_icon.png")};
 static const Texture portrait{FsUtils::GetResourcePath(U"ui/menu_vertical_icon.png")};
 const auto& icon=vertical?portrait:custom;
 icon.scaled(rect.w*.875/Max(icon.width(),icon.height())).drawAt(rect.center());
}
}
