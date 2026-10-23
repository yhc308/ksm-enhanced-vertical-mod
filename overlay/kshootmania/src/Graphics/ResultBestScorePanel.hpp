#pragma once
#include "Graphics/ExScorePanel.hpp"
#include "HighScore/HighScoreInfo.hpp"

// Captured before saving: rendering must never re-read the updated best record.
struct ResultBestScoreComparison
{
    bool visible = false;
    int64 bestScore = 0, bestEx = 0, scoreDelta = 0, exDelta = 0;
    static ResultBestScoreComparison Capture(const HighScoreInfo& previous, const MusicGame::PlayResult& current)
    {
        const int64 score = previous.score(current.playOption.gaugeType);
        const int64 ex = previous.exScore(current.playOption.gaugeType).value_or(0);
        return {current.playOption.shouldSaveScore(), score, ex, static_cast<int64>(current.score)-score, current.exScore-ex};
    }
};

namespace ResultBestScorePanel {
inline std::weak_ptr<noco::Node> captureNode;
inline void CaptureLayout(){
    if(const auto node=captureNode.lock()){
        const auto rect=node->transformedQuad().boundingRect();JSON layout;
        layout[U"x"]=rect.x;layout[U"y"]=rect.y;layout[U"width"]=rect.w;layout[U"height"]=rect.h;layout[U"visible"]=node->activeSelf();
        layout.save(FsUtils::GetResourcePath(U"result-best-layout.json"));
    }
}
inline Image Compose(const ResultBestScoreComparison& data)
{
    const Image background{SkinPreset::Resolve(U"result_bestscore_checker.png")};
    const Image digits{SkinPreset::Resolve(U"num0.png")}, symbols{SkinPreset::Resolve(U"font.png")};
    if(background.isEmpty() || digits.isEmpty() || symbols.isEmpty()) return {};
    // Two transparent pixels preserve the glow at every edge of the supplied panel.
    Image result{248,214,Color{0,0}}, exMask{248,214,Color{0,0}};
    const Image panel=background.scaled(244,210);
    for(int y=0;y<210;++y)for(int x=0;x<244;++x)result[y+2][x+2]=panel[y][x];
    const auto stamp=[&](const Image& glyph,int x,int y,double opacity,bool ex){
        for(int j=0;j<glyph.height();++j)for(int i=0;i<glyph.width();++i){
            const int xx=x+i+2-5,yy=y+j+2;if(xx<0 || yy<0 || xx>=248 || yy>=214)continue;
            Color c=glyph[j][i];
            if(ex){
                const double ink=Clamp((Max(c.r,Max(c.g,c.b))-32)/96.0,0.0,1.0);
                exMask[yy][xx].a=Max(exMask[yy][xx].a,static_cast<uint8>(Round(c.a*ink*opacity)));
            }else{c.a=static_cast<uint8>(Round(c.a*opacity));ExScorePanel::Over(result[yy][xx],c);}
        }
    };
    const auto number=[&](int64 value,int top,bool padded,bool ex,int leftLimit=60){
        const String text=Format(value);const size_t length=padded?Max(size_t{8},text.size()):text.size();
        const double step=Min(21.0,(228.0-leftLimit)/static_cast<double>(length));
        const int width=Max(1,static_cast<int>(Round(step*.94)));
        const int left=228-static_cast<int>(Round(step*length));
        const size_t zeros=length-text.size();
        for(size_t i=0;i<length;++i){
            const int digit=i<zeros?0:text[i-zeros]-U'0';
            const Image glyph=digits.clipped(Rect{0,digit*(digits.height()/10),digits.width(),digits.height()/10}).scaled(width,22);
            stamp(glyph,left+static_cast<int>(Round(i*step)),top,i<zeros?.25:1.0,ex);
        }
        return std::pair<int,double>{left,step};
    };
    const auto sign=[&](int64 delta,int x,int y,bool ex){
        if(delta==0)return;
        // font.noco: row 4 is : < > ( ) " ' + - / . Use its exact cell layout.
        const int cellW=symbols.width()/10,cellH=cellW;
        const int trim=static_cast<int>(Round(cellW*13.0/64.0));
        const Image glyph=symbols.clipped(Rect{(delta>0?7:8)*cellW+trim,4*cellH,Max(1,cellW-static_cast<int>(Round(cellW*27.0/64.0))),cellH}).scaled(18,static_cast<int32>(Round(22*1.2)));
        stamp(glyph,x,y-2,1.0,ex);
    };
    number(data.bestScore,44,true,false);number(data.scoreDelta<0?-data.scoreDelta:data.scoreDelta,76,true,false);
    sign(data.scoreDelta,35,76,false);
    number(data.bestEx,137,false,true);
    const auto [exDeltaLeft,exDeltaStep]=number(data.exDelta<0?-data.exDelta:data.exDelta,169,false,true,104);
    // The immediately preceding number cell holds the sign, regardless of digit count.
    sign(data.exDelta,static_cast<int>(Round(exDeltaLeft-exDeltaStep)),169,true);
    constexpr double weights[5]={.054489,.244201,.402620,.244201,.054489};
    for(int y=0;y<214;++y)for(int x=0;x<248;++x){
        double alpha=0;
        for(int j=-2;j<=2;++j)for(int i=-2;i<=2;++i)
            if(y+j>=0 && y+j<214 && x+i>=0 && x+i<248)alpha+=exMask[y+j][x+i].a*weights[j+2]*weights[i+2];
        ExScorePanel::Over(result[y][x],Color{142,60,0,static_cast<uint8>(Round(alpha*.72*(1-exMask[y][x].a/255.0)))});
        ExScorePanel::Over(result[y][x],Color{255,227,12,exMask[y][x].a});
    }
    return result;
}
inline void Set(noco::Canvas& canvas,ResultBestScoreComparison data)
{
    const bool fixture=FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture"));
    if(fixture){
        const auto& args=System::GetCommandLineArgs();
        if(args.contains(U"--best-checker-positive"))data={true,159201,6600,56665,30};
        if(args.contains(U"--best-checker-negative"))data={true,10000000,6800,-12345,-102};
        if(args.contains(U"--best-checker-zero"))data={true,0,0,0,0};
        if(args.contains(U"--best-checker-large"))data={true,10000000,19999,0,-19999};
    }
    const auto node=canvas.findByName(U"ResultBestScorePanel",noco::RecursiveYN::Yes,noco::IncludeSubCanvasYN::Yes);
    if(!node)return;
    if(fixture)captureNode=node;
    node->setActive(data.visible);if(!data.visible)return;
    if(const auto sprite=node->getComponent<noco::Sprite>())sprite->setTexture(Texture{Compose(data)});

}
}
