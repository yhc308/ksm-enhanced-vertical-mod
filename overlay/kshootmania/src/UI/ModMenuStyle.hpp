#pragma once
#include "Common/AssetManagement.hpp"
#include "Common/SkinPreset.hpp"
#include "Vertical/MenuContentLayout.hpp"
#include "Input/InputUtils.hpp"
#include "I18n/I18n.hpp"

// Shared presentation for the three mod menus. All image resources retain skin lookup.
namespace ModMenuStyle {
inline double Unit(){return Min(Scene::Height()/720.0,Scene::Width()/960.0);}
inline void Text(StringView text,const RectF& rect,double size,ColorF color=Palette::White){
    const auto font=AssetManagement::SystemFont();
    size=Min(size,rect.w/Max(1.0,font(text).region(1.0).w));
    font(text).drawAt(TextStyle::Outline(0,.12,ColorF{0,.9}),size,rect.center(),color);
}
inline std::pair<int32,int32> Trim(char32 c){
    if(c==U' ')return {25,25};if(c==U'I')return {15,16};if(c==U'M'||c==U'W')return {5,4};
    if(c==U'N'||c==U'S')return {7,7};if(c==U'T')return {7,8};if(c==U'U')return {8,7};if(c==U'J')return {7,7};
    if(c==U'-')return {14,13};return {8,6};
}
inline double TitleHeight(){
    double referenceUnits=0;
    for(char32 c:U"JUDGE TIMING SETTING"_sv){const auto [l,r]=Trim(c);referenceUnits+=64-l-r+4;}
    const bool portrait=Scene::Height()>Scene::Width();
    return Min(20*(portrait?Vertical::MenuContent::Scale():Unit()),Scene::Width()*(portrait?.48:.34)*48/referenceUnits);
}
inline Vec2 TitlePosition(double portraitTop=110){
    // Keep the title cap centered on the normal close-button row in both modes.
    if(Scene::Height()>Scene::Width()){
        const auto view=Vertical::Viewport::Fit(Scene::Width(),Scene::Height());
        return {view.x(12),view.y(portraitTop+10)};
    }
    return {12*Unit(),12*Unit()};
}
inline void Title(StringView text,Vec2 pos,double maxWidth,double height=0){
    if(height<=0)height=TitleHeight();
    constexpr StringView chars=U"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!?.,:<>()\"'+-/ ";
    double units=0;for(char32 c:text){const auto [l,r]=Trim(c);units+=64-l-r+4;}
    // Use one font scale for every menu; reserve the same half-screen title area.
    double referenceUnits=0;
    for(char32 c:U"JUDGE TIMING SETTING"_sv){const auto [l,r]=Trim(c);referenceUnits+=64-l-r+4;}
    const double widthLimit=Min(maxWidth,Scene::Width()*(Scene::Height()>Scene::Width()?.48:.34));
    const double scale=Min(height/48.0,widthLimit/Max(referenceUnits,units));
    // Registry lookups are cached and refreshed by SkinPreset::Activate; no per-frame file IO.
    const Texture shadow=TextureAsset(U"font_shadow.png"),title=TextureAsset(U"font_title.png");
    for(const auto& texture:{shadow,title}){
        double x=pos.x;
        for(char32 c:text){const auto index=chars.indexOf(c);const auto [l,r]=Trim(c);
            if(c!=U' '&&index!=StringView::npos)texture((index%10)*64,(index/10)*64,64,64).scaled(scale).draw(x-l*scale,pos.y-10*scale);
            x+=(64-l-r+4)*scale;
        }
    }
}
// Draw-only feedback uses exactly the same rectangles as click hit testing.
inline bool Hover(const RectF& rect,bool enabled=true){
    if(!enabled || !InputUtils::IsUIMouseInputEnabled() || !Window::GetState().focused || !rect.mouseOver())return false;
    Cursor::RequestStyle(CursorStyle::Hand);
    rect.draw(ColorF{1,MouseL.pressed()?.28:.12}).drawFrame(2*Unit(),ColorF{1,.95});
    return true;
}
inline RectF BackRect(){const double s=Scene::Height()/480.0;return {0,0,48*s,48*s};}
inline RectF CloseRect(double portraitTop=110){
    if(Scene::Height()>Scene::Width()){
        const auto view=Vertical::Viewport::Fit(Scene::Width(),Scene::Height());
        return {view.x(644),view.y(portraitTop+14),48*view.scale,48*view.scale};
    }
    const double u=Unit();return {Scene::Width()-64*u,20*u,40*u,40*u};
}
inline void IconPanel(const RectF& rect,bool selected);
inline void CloseButton(const RectF& rect,bool enabled=true,bool focused=false){
    const bool hovered=enabled&&InputUtils::IsUIMouseInputEnabled()&&Window::GetState().focused&&rect.mouseOver();
    IconPanel(rect,enabled&&(hovered||focused));
    if(hovered)Cursor::RequestStyle(CursorStyle::Hand);
    const double inset=rect.w*.22,stroke=rect.w/20;
    const ColorF color=enabled?ColorF{1}:ColorF{.4};
    Line{rect.tl()+Vec2{inset,inset},rect.br()-Vec2{inset,inset}}.draw(stroke,color);
    Line{rect.tr()+Vec2{-inset,inset},rect.bl()+Vec2{inset,-inset}}.draw(stroke,color);
}
inline void Panel(const RectF& rect){rect.draw(ColorF{.06}).drawFrame(Unit(),ColorF{.5});}
inline RectF DialogPanel(int count){const double u=Unit();const Vec2 c=Scene::Center();return {c.x-240*u,c.y-(count==3?122:85)*u,480*u,(count==3?244:170)*u};}
inline RectF DialogButton(int count,int index){const double u=Unit();const Vec2 c=Scene::Center();
    if(count==2)return {c.x+(index==0?-145:25)*u,c.y+12*u,120*u,44*u};
    return {c.x-150*u,c.y+(-30+44*index)*u,300*u,40*u};
}
inline void Dialog(StringView message,const Array<StringView>& choices,int selected){
    const double u=Unit();Scene::Rect().draw(ColorF{0,.6});const auto p=DialogPanel(static_cast<int>(choices.size()));Panel(p);
    Text(message,{p.x+16*u,p.y+20*u,p.w-32*u,40*u},26*u);
    for(int i=0;i<static_cast<int>(choices.size());++i){const auto b=DialogButton(static_cast<int>(choices.size()),i);if(i==selected)b.draw(ColorF{1,.18}).drawFrame(u,Palette::White);Hover(b);Text(choices[i],b,23*u);}
}
inline void Footer(StringView first,StringView second=U""){
    const double u=Unit(),y=Scene::Height()>Scene::Width()?Vertical::MenuContent::Rect().br().y-26*Vertical::MenuContent::Scale():Scene::Height()-36*u;
    for(const auto& [text,line]:std::array<std::pair<StringView,int>,2>{{{first,0},{second,1}}}){
        if(text.isEmpty())continue;const auto font=AssetManagement::SystemFont();const double size=Min(15*u,(Scene::Width()-48*u)/Max(1.0,font(text).region(1.0).w));
        font(text).draw(TextStyle::Outline(0,.12,ColorF{0,.9}),size,Vec2{24*u,y+line*18*u-size*.5},Palette::White);
    }
}
inline const Texture& IconGlow(){
    static const Texture glow=[] {
        Image mask{Size{96,96},Color{255,255,255,0}};
        for(int y=0;y<96;++y)for(int x=0;x<96;++x){
            const double qx=Abs(x-47.5)-22,qy=Abs(y-47.5)-22;
            const double d=std::hypot(Max(qx,0.0),Max(qy,0.0))+Min(Max(qx,qy),0.0)-10;
            mask[y][x].a=static_cast<uint8>(Clamp(.5-d,0.0,1.0)*255);
        }
        Image blur=mask.gaussianBlurred(15);
        for(int y=0;y<96;++y)for(int x=0;x<96;++x)blur[y][x].a=static_cast<uint8>(Min(220.0,blur[y][x].a*2.5)*(1-mask[y][x].a/255.0));
        return Texture{blur};
    }();return glow;
}
inline void IconPanel(const RectF& rect,bool selected){
    const double scale=rect.w/64;
    if(selected)IconGlow().resized(96*scale,96*scale).draw(rect.pos-Vec2{16,16}*scale);
    rect.rounded(10*scale).draw(ColorF{0,selected?.56:.35});
    if(selected)rect.rounded(10*scale).drawFrame(1.2*scale,ColorF{1,.95});
}
}
