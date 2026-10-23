#pragma once
#include "MusicGame/GameMain.hpp"
#include "Vertical/DiagnosticInputReplay.hpp"
#include "kson/IO/KsonIO.hpp"
#include <deque>
namespace ApsVideoFixture {
inline void Run() {
#ifdef _WIN32
    using namespace MusicGame;
    if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture")))throw Error{U"Isolated APS video fixture required"};
    const auto render=reinterpret_cast<DWORD(WINAPI*)(DWORD,float*)>(::GetProcAddress(::GetModuleHandleW(L"bassasio.dll"),"FakeAsioRender"));
    if(!render || !ksmaudio::IsASIOActive())throw Error{U"APS video requires mock ASIO"};
    struct ClockScope {~ClockScope(){GaugePresentation::FixtureStep().reset();GaugePresentation::FixtureChartTime().reset();}} clockScope;
    GaugePresentation::FixtureStep()=1.0/60;
    const bool vertical=System::GetCommandLineArgs().contains(U"--aps-video-portrait");
    const Size size=vertical?Size{720,1280}:Size{1280,720};Window::Resize(size);Scene::Resize(size);
    TextWriter report{FsUtils::GetResourcePath(U"aps-video.txt")};
    const auto check=[&](bool ok,StringView text){if(!ok)throw Error{String{text}};report.writeln(U"PASS "+String{text});};
    kson::ChartData chart;chart.meta.title="APS TRANSITION REPLAY";chart.meta.artist="Deterministic input / no score save";chart.meta.dispBPM="240";chart.meta.level=15;
    chart.beat.bpm.emplace(0,240);chart.beat.timeSig.emplace(0,kson::TimeSig{});chart.audio.bgm.filename="replay.ogg";
    chart.meta.jacketFilename="replay-jacket.jpg";
    for(int group=0;group<256;++group)for(int lane=0;lane<4;++lane)chart.note.bt[lane][group*kson::kResolution4/16]=kson::Interval{};
    check(kson::SaveKsonChartData("aps-replay.kson",chart)==kson::ErrorType::None,U"replay chart created in isolated fixture");
    GameCreateInfo info;info.chartFilePath=U"aps-replay.kson";info.playOption.gaugeType=GaugeType::kHardGauge;info.playOption.apsEnabled=true;
    info.playOption.movieEnabled=false;info.playOption.showLayer=false;
    GameMain game{info};game.start();
    Vertical::DiagnosticInputReplay::Session input;
    const Rect crop=vertical?Rect{565,310,154,450}:Rect{850,130,180,440};
    const Size zoomSize=crop.size*2;
    const FilePath fullPath=FsUtils::GetResourcePath(vertical?U"aps-transition-portrait.mp4":U"aps-transition-landscape.mp4");
    const FilePath zoomPath=FsUtils::GetResourcePath(vertical?U"aps-transition-portrait-zoom.mp4":U"aps-transition-landscape-zoom.mp4");
    VideoWriter full{fullPath,size,60},zoom{zoomPath,zoomSize,60};
    check(full.isOpen() && zoom.isOpen(),U"60 FPS MP4 encoders opened");
    RenderTexture output{size,HasDepth::Yes};std::deque<Image> preRoll;std::vector<float> pcm(1470);
    bool switched=false;int postFrames=0,frames=0;double switchTime=0;int successBefore=0;const Font caption{14};
    TextWriter trace{FsUtils::GetResourcePath(U"aps-video-frames.csv")};trace.writeln(U"frame,time,type,actual,display,dodge,opacity,burn");
    const auto write=[&](const Image& image){check(full.writeFrame(image),U"full video frame encoded");check(zoom.writeFrame(image.clipped(crop).scaled(zoomSize)),U"zoom extracted from original full frame");++frames;};
    for(int tick=0;tick<1400 && postFrames<181;++tick) {
        if(!System::Update())throw Error{U"APS video window closed"};
        const double time=(tick+1)/60.0-3.4;GaugePresentation::FixtureChartTime()=time;
        check(render(735,pcm.data())==735*8,U"mock audio callback advanced at 60 Hz");
        uint32 held=0;const int group=static_cast<int>(std::round(time/.0625));
        if(time>=-.01 && group>=0 && group<256 && Abs(time-group*.0625)<=1.0/120+.00001 && (time<8 || switched))
            for(int lane=0;lane<4;++lane)held|=Vertical::DiagnosticInputReplay::Mask(kButtonBT_A+lane);
        Vertical::DiagnosticInputReplay::Frame(held);game.update();const auto& view=game.previewView();
        const bool event=!switched && view.effectiveGaugeType==GaugeType::kNormalGauge;
        if(event){switched=true;switchTime=time;successBefore=game.playResult().comboStats.critical;check(view.gaugePercentage>15,U"real fallback contains accumulated NORMAL reserve");for(const auto& image:preRoll)write(image);preRoll.clear();}
        if(time>=6.5 || switched){
            {const ScopedRenderTarget3D target3D{output.clear(Palette::Black)};const ScopedRenderTarget2D target2D{output};game.draw();Graphics3D::Flush();Graphics2D::Flush();
                RectF{0,size.y-28.0,static_cast<double>(size.x),28}.draw(ColorF{0,.8});
                caption(U"APS REPLAY 60fps | t={:.3f}s | actual={:.2f}% display={:.2f}%"_fmt(switched?time-switchTime:-1.0,view.gaugePercentage,view.gaugeVisual.percent)).draw(8,size.y-25,Palette::White);Graphics2D::Flush();}
            Image image;output.readAsImage(image);
            if(switched){write(image);trace.writeln(U"{},{},{},{},{},{},{},{}"_fmt(frames-1,time-switchTime,static_cast<int>(view.effectiveGaugeType),view.gaugePercentage,view.gaugeVisual.percent,view.gaugeVisual.dodge,view.gaugeVisual.bannerOpacity,view.gaugeVisual.burn));
                if(postFrames==0 || postFrames==12 || postFrames==24 || postFrames==36 || postFrames==42 || postFrames==48 || postFrames==60 || postFrames==72)image.save(FsUtils::GetResourcePath(U"aps-video-stage-{}.png"_fmt(postFrames)));
                ++postFrames;
            }else{preRoll.push_back(std::move(image));if(preRoll.size()>60)preRoll.pop_front();}
        }
    }
    check(switched && frames==241 && successBefore>=480,U"real input replay records one second before and three seconds after actual APS switch");
    check(!game.playResult().isHardFailed,U"recording never takes HARD failure lock path");
    check(Abs(game.previewView().gaugeVisual.percent-game.previewView().gaugePercentage)<1e-8,U"display converges to actual NORMAL during recorded play");
    game.terminate();full.close();zoom.close();
    for(const auto path:{fullPath,zoomPath}){
        VideoReader reader{path};check(reader.isOpen() && Abs(reader.getFPS()-60)<.01 && reader.getFrameCount()==static_cast<size_t>(frames),U"recorded MP4 reopens with expected FPS and frame count");
        check(reader.getSize()==(path==fullPath?size:zoomSize),U"recorded MP4 retains native or crop resolution");
        Image decoded;int count=0;while(reader.readFrame(decoded))++count;check(count==frames,U"every recorded MP4 frame decodes");
    }
    report.writeln(U"TOTAL APS video passed frames={} switchTime={} full={} zoom={}"_fmt(frames,switchTime,fullPath,zoomPath));
#endif
}
}
