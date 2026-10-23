#pragma once
#include <NocoUI.hpp>
#include "Common/SkinPreset.hpp"
#include "Common/FsUtils.hpp"
#include "MusicGame/Judgment/ExScore.hpp"
namespace ExScorePanel {
inline std::weak_ptr<noco::Node> captureNode;
inline void CaptureLayout(StringView name){
    if(const auto node=captureNode.lock()){
        const auto rect=node->transformedQuad().boundingRect();JSON data;
        data[U"x"]=rect.x;data[U"y"]=rect.y;data[U"width"]=rect.w;data[U"height"]=rect.h;data[U"visible"]=node->activeSelf();
        data.save(FsUtils::GetResourcePath(String{name}+U"-ex-layout.json"));
    }
}
inline void Over(Color& dst,Color src){
    const double a=src.a/255.0,b=dst.a/255.0,out=a+b*(1-a);
    if(out<=0){dst=Color{0,0};return;}
    dst=Color{static_cast<uint8>(Round((src.r*a+dst.r*b*(1-a))/out)),static_cast<uint8>(Round((src.g*a+dst.g*b*(1-a))/out)),static_cast<uint8>(Round((src.b*a+dst.b*b*(1-a))/out)),static_cast<uint8>(Round(out*255))};
}
inline Image Compose(int64 score,int64 maximum){
    const int kind=MusicGame::Judgment::ExPanelKind(score,maximum);
    if(!kind)return {};
    const Image background{SkinPreset::Resolve(U"msel_exscore_{}.png"_fmt(kind))};
    const Image font{SkinPreset::Resolve(U"num0.png")};
    if(background.isEmpty() || font.isEmpty())return {};
    Image result{228,46,Color{0,0}},mask{228,46,Color{0,0}};
    const Image panel=background.scaled(222,40);
    for(int y=0;y<40;++y)for(int x=0;x<222;++x)result[y+3][x+3]=panel[y][x];
    const auto glyph=[&](int digit,int x,int y,int width,int height){
        const int cell=font.height()/10;
        const Image im=font.clipped(Rect{0,digit*cell,font.width(),cell}).scaled(width,height);
        for(int j=0;j<height;++j)for(int i=0;i<width;++i){
            const int xx=x+i+3,yy=y+j+3;if(xx<0 || yy<0 || xx>=228 || yy>=46)continue;
            const Color c=im[j][i];const double ink=Clamp((Max(c.r,Max(c.g,c.b))-32)/96.0,0.0,1.0);
            mask[yy][xx].a=Max(mask[yy][xx].a,static_cast<uint8>(Round(c.a*ink)));
        }
    };
    const auto number=[&](int64 value,int anchor,int top,int slot,int height,double normalStep,bool alignLeft=false){
        const String text=Format(value);const double step=Min(normalStep,slot/static_cast<double>(text.size()));
        const int width=Max(1,static_cast<int>(Round(step*.82)));
        const int left=alignLeft ? anchor : anchor-static_cast<int>(Round(step*text.size()));
        for(size_t i=0;i<text.size();++i)glyph(text[i]-U'0',left+static_cast<int>(Round(i*step)),top,width,height);
    };
    // Slash is already embedded in the supplied PNG; draw only the two numbers.
    number(score,163,5,41,8,9);number(maximum,178,5,39,8,9,true);
    if(kind==1){
        const auto percent=MusicGame::Judgment::ExPercentHundredths(score,maximum);
        number(percent/100,126,18,34,16,17);number(percent%100,170,18,34,16,17);
        // Fractions always have two digits, unlike the non-padded score fields.
        if(percent%100<10)glyph(0,136,18,14,16);
        for(int y=31;y<33;++y)for(int x=128;x<130;++x)mask[y+3][x+3].a=255;
    }else if(kind==2){
        const int64 missing=maximum-score;const double step=Min(18.0,84.0/Format(missing).size());
        const int right=Max(156,static_cast<int>(Round(120+step*Format(missing).size())));
        number(missing,right,18,84,16,step);
        for(int y=26;y<28;++y)for(int x=108;x<115;++x)mask[y+3][x+3].a=255;
    }
    // Radius 3px, zero spread: blur the alpha mask without dilating it.
    constexpr double weights[7]={0.004433,0.054006,0.242036,0.39905,0.242036,0.054006,0.004433};
    for(int y=0;y<46;++y)for(int x=0;x<228;++x){
        double alpha=0;
        for(int j=-3;j<=3;++j)for(int i=-3;i<=3;++i)
            if(y+j>=0 && y+j<46 && x+i>=0 && x+i<228)alpha+=mask[y+j][x+i].a*weights[j+3]*weights[i+3];
        alpha*=.72*(1-mask[y][x].a/255.0);
        Over(result[y][x],Color{172,117,0,static_cast<uint8>(Round(alpha))});
        Over(result[y][x],Color{255,227,12,mask[y][x].a});
    }
    return result;
}
inline void Set(noco::Canvas& canvas,Optional<int64> score,int64 maximum){
    // Explicit, isolated capture data never touches actual chart records.
    static const bool fixture=FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture"));
    if(fixture){
        const auto& args=System::GetCommandLineArgs();
        if(args.contains(U"--ex-example-percent")){score=6630;maximum=6800;}
        if(args.contains(U"--ex-example-minus")){score=6698;maximum=6800;}
        if(args.contains(U"--ex-example-max")){score=6800;maximum=6800;}
        if(args.contains(U"--ex-example-small")){score=8;maximum=8;}
        if(args.contains(U"--ex-example-none"))score=none;
    }
    const auto node=canvas.findByName(U"ExScorePanel",noco::RecursiveYN::Yes,noco::IncludeSubCanvasYN::Yes);
    if(!node)return;
    if(fixture)captureNode=node;
    const bool visible=score && MusicGame::Judgment::ExPanelKind(*score,maximum)!=0;
    node->setActive(visible);if(!visible)return;
    const auto& skin=SkinPreset::Current();
    const String key=skin.folder+U"/{}/{}/{}"_fmt(static_cast<int>(skin.missing),*score,maximum);
    static HashTable<String,Texture> cache;
    auto it=cache.find(key);
    if(it==cache.end()){
        if(cache.size()>=128)cache.clear();
        it=cache.emplace(key,Texture{Compose(*score,maximum)}).first;
    }
    if(const auto sprite=node->getComponent<noco::Sprite>())sprite->setTexture(it->second);
}
}
