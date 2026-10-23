#pragma once
#include "Scenes/Title/JudgeSettingOverlay.hpp"
#include "Scenes/Title/ModMenuButtons.hpp"
#include "Scenes/Title/TitleMenu.hpp"
#include "Vertical/DiagnosticInputReplay.hpp"
#include "kson/IO/KsonIO.hpp"
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif

namespace JudgeSettingFixture {
inline void Performance(){
    using namespace MusicGame;
    TextWriter report{FsUtils::GetResourcePath(U"judge-setting-performance.csv")};report.writeln(U"width,height,mode,frames,mean_ms,p99_ms,over_8_333ms");
    struct ClockScope{~ClockScope(){GaugePresentation::FixtureStep().reset();GaugePresentation::FixtureChartTime().reset();}}clock;
    GaugePresentation::FixtureStep()=1.0/300;GaugePresentation::FixtureChartTime()=2.5;
    Vertical::DiagnosticInputReplay::Session input;
    for(const Size size:{Size{1280,720},Size{1080,1920}}){
        ConfigIni::SetBool(U"vertical_enabled",size.y>size.x);GameRender::Scope context{{size,size.y>size.x}};Window::Resize(size);Scene::Resize(size);
        for(int mode=0;mode<3;++mode){
            JudgeSettingOverlay overlay;if(mode==2)overlay.requestMenu();
            RenderTexture output{size,HasDepth::Yes};Array<double> samples;
            for(int i=0;i<1200;++i){
                if(!System::Update())throw Error{U"Benchmark closed"};
                if(i>=240)samples.push_back(Scene::DeltaTime()*1000);
                Vertical::DiagnosticInputReplay::Frame(0);overlay.update(1.0/300);
                {const ScopedRenderTarget3D t3{output.clear(Palette::Black)};const ScopedRenderTarget2D t2{output};if(mode)overlay.draw();else overlay.drawWorld();Graphics3D::Flush();Graphics2D::Flush();}
                output.draw();
            }
            const double mean=std::accumulate(samples.begin(),samples.end(),0.0)/samples.size();samples.sort();
            const auto over=std::count_if(samples.begin(),samples.end(),[](double ms){return ms>8.333;});
            report.writeln(U"{},{},{},{},{},{},{}"_fmt(size.x,size.y,mode,samples.size(),mean,samples[static_cast<size_t>(samples.size()*.99)],over));
        }
    }
}
inline void Run(){
    using namespace MusicGame;using namespace MusicGame::Judgment;
    if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture")))throw Error{U"Isolated judge-setting fixture required"};
    if(System::GetCommandLineArgs().contains(U"--judge-setting-performance")){Performance();return;}
    TextWriter report{FsUtils::GetResourcePath(U"judge-setting-test.txt")};int checks=0;
    const auto check=[&](bool ok,StringView name){if(!ok)throw Error{String{name}};++checks;report.writeln(U"PASS "+String{name});};
    {
        JudgeSettingSession state;state.chip(JudgmentResult::kCritical,10);state.chip(JudgmentResult::kNearSlow,-30);state.chip(JudgmentResult::kError,150);state.finishFrame();
        check(state.cumulative.count==2 && state.recentMean() && *state.recentMean()==-10,U"mean includes critical/near only");
        check(state.last && *state.last==150,U"last input includes measured error and largest simultaneous magnitude");
        state.advance(9.999);check(state.recent.size()==2,U"10 second window retains younger samples");state.advance(.001);check(state.recent.empty()&&state.recentMean()==Optional<double>{-10}&&state.last==Optional<double>{150},U"idle expiration prunes the internal window but holds both displayed readings");
        state.chip(JudgmentResult::kError,none);state.finishFrame();check(state.recentMean()==Optional<double>{-10}&&state.last==Optional<double>{150},U"automatic misses do not refresh input-only displays");
        state.clearRecent();check(state.recentMean()==Optional<double>{-10}&&state.last==Optional<double>{150},U"menu return preserves displayed readings until a new measured note");
        state.clearRecent();state.advance(0);state.chip(JudgmentResult::kCritical,20);state.chip(JudgmentResult::kCritical,-20);state.finishFrame();check(state.last&&*state.last==20,U"equal simultaneous magnitudes preserve lane order");
        state.menu=true;state.chip(JudgmentResult::kCritical,0);state.tick(JudgmentResult::kError);check(state.cumulative.count==4,U"menu inputs never enter measurements");
        state.menu=false;state.values.mode=FastSlowMode::kNew;check(state.navigate(5,-1)==0 && state.navigate(0,1)==5,U"panels form a column-major list");
        state.values.mode=FastSlowMode::kHide;check(state.navigate(5,-1)==0,U"disabled NEW+ settings cannot receive focus");
        state.change(5,1);check(state.cumulative.count==0 && state.last==Optional<double>{20} && state.counts.fastCritical==0,U"timing draft change clears measurement populations while preserving the input-only display");
        state.values.mode=FastSlowMode::kNew;
        state.values.hud.display=TimingMsDisplay::kOff;state.values.hud.color=TimingMsColor::kOff;
        for(int i=0;i<4;++i){
            check(TimingDisplayCursor(state.values.hud.display)==i && static_cast<int32>(state.values.hud.color)==i,U"judge settings use OFF NEAR CRIT ALL and OFF NEAR CRIT sixteen-ms order");
            state.change(3,1);state.change(2,1);
        }
        check(state.values.hud.display==TimingMsDisplay::kAll && state.values.hud.color==TimingMsColor::kSixteenMs,U"last display and color choices clamp at their upper bounds");
        state.change(3,-20);state.change(2,-20);
        check(state.values.hud.display==TimingMsDisplay::kOff && state.values.hud.color==TimingMsColor::kOff,U"first display and color choices clamp at OFF");
        state.values.mode=FastSlowMode::kNew;state.chip(JudgmentResult::kCritical,5);state.change(1,1);check(state.cumulative.count==1,U"palette edit preserves measurement population");
        PlayOption offset;
        offset.globalOffsetMs=9;offset.audioProcDelayMs=3;offset.judgeSetting=std::make_shared<JudgeSettingSession>();
        for(const int visual:{-100,0,100})for(const int inputDelay:{-25,0,25})for(const int laser:{-17,0,17}){
            offset.judgeSetting->values.visual=visual;offset.judgeSetting->values.input=inputDelay;offset.judgeSetting->values.laser=laser;
            const double displayed=4.0+visual/1000.0;
            const double button=displayed-offset.effectiveInputDelayMs()/1000.0;
            const double laserTime=button-offset.effectiveLaserInputDelayMs()/1000.0;
            check(Abs(button-(4.0-inputDelay/1000.0))<1e-9,U"visual correction cancels from button judgment clock");
            check(Abs(laserTime-(4.0-(inputDelay+laser)/1000.0))<1e-9,U"laser correction remains relative to shared judgment correction");
            check(offset.effectiveGlobalOffsetMs()==9-visual&&offset.effectiveAudioProcDelayMs()==3+visual,U"visual correction preserves original audio and effect offset relationship");
        }
        PlayOption options;options.judgeSetting=std::make_shared<JudgeSettingSession>();check(!options.shouldSaveScore()&&!options.effectiveApsEnabled(),U"settings session never writes scores or activates APS");
    }
    {
        kson::ChartData chart;chart.beat.bpm.emplace(0,120);chart.note.bt[0].emplace(0,kson::Interval{kson::kResolution4});
        const auto cache=kson::CreateTimingCache(chart.beat);PlayOption options;options.judgeSetting=std::make_shared<JudgeSettingSession>();
        JudgmentMain judgment{chart,cache,options,none,GameMode::kNormal};GameStatus game;ViewStatus view;Vertical::DiagnosticInputReplay::Session input;
        const auto step=[&](double time,uint32 held){
            Vertical::DiagnosticInputReplay::Frame(held);game.currentTimeSec=game.currentTimeSecForButtonJudgment=game.currentTimeSecForLaserJudgment=time;
            game.currentPulse=game.currentPulseForButtonJudgment=game.currentPulseForLaserJudgment=kson::SecToPulse(time,chart.beat,cache);
            judgment.update(chart,game,view);
        };
        const auto key=Vertical::DiagnosticInputReplay::Mask(kButtonBT_A);step(0,key);options.judgeSetting->menu=true;step(.2,key);step(.8,key);
        const auto before=options.judgeSetting->counts.perfectCritical;options.judgeSetting->menu=false;step(.81,key);
        check(options.judgeSetting->counts.perfectCritical==before,U"held long note cannot auto-succeed across menu return");
        step(.82,0);step(.83,key);step(1.1,key);
        check(options.judgeSetting->counts.perfectCritical>before&&options.judgeSetting->cumulative.count==0,U"fresh long press can resume ticks without inventing timing samples");
        judgment.resetSettingsLoop(chart);game=GameStatus{};step(0,0);step(.4,0);
        check(options.judgeSetting->cumulative.count==0,U"low-FPS long misses do not enter timing mean");
    }
    const auto originalSkin=SkinPreset::Current();auto hiddenSkin=originalSkin;hiddenSkin.indicatorLandscape=false;hiddenSkin.indicatorPortrait=SkinPreset::IndicatorPortrait::Off;SkinPreset::Activate(hiddenSkin);
    ConfigIni::SetInt(ConfigIni::Key::kJudgeColor,0);ConfigIni::SetInt(ConfigIni::Key::kShowFastSlow,2);ConfigIni::SetInt(ConfigIni::Key::kTimingMsDisplay,1);ConfigIni::SetInt(ConfigIni::Key::kInputDelay,0);ConfigIni::SetInt(ConfigIni::Key::kVisualOffset,0);ConfigIni::SetInt(ConfigIni::Key::kLaserInputDelay,0);
    struct ClockScope{~ClockScope(){GaugePresentation::FixtureStep().reset();GaugePresentation::FixtureChartTime().reset();}}clockScope;
    GaugePresentation::FixtureStep()=1.0/60;
    for(const Size size:{Size{640,480},Size{1280,720},Size{1920,1080},Size{720,1280},Size{1080,1920}}){
        const bool vertical=size.y>size.x;ConfigIni::SetBool(U"vertical_enabled",vertical);GameRender::Scope context{{size,vertical}};Window::Resize(size);Scene::Resize(size);
        {
            auto shown=SkinPreset::Current();shown.indicatorLandscape=true;shown.indicatorPortrait=SkinPreset::IndicatorPortrait::Both;SkinPreset::Activate(shown);
            MusicGame::Graphics::PlayJudgeIndicator ordinary,settings;ViewStatus view;view.detailedJudgments={123,234,345,456,567,678,789};view.inputTiming.add(JudgmentResult::kCritical,16.1);
            ordinary.update(view,JudgeColor::kFastBlue);settings.update(view,JudgeColor::kFastBlue,true);
            RenderTexture nativeImage{size},settingImage{size};
            {const ScopedRenderTarget2D target{nativeImage.clear(Palette::Black)};ordinary.draw();Graphics2D::Flush();}
            const auto viewport=Vertical::Viewport::Fit(size.x,size.y);
            {const ScopedRenderTarget2D target{settingImage.clear(Palette::Black)};settings.drawSettings(vertical?RectF{viewport.x(0),viewport.y(963),720*viewport.scale,242*viewport.scale}:RectF{},MusicGame::Graphics::PlayJudgeIndicator::MiniRect());Graphics2D::Flush();}
            Image a,b;nativeImage.readAsImage(a);settingImage.readAsImage(b);int maximum=0;
            for(int y=0;y<size.y;++y)for(int x=0;x<size.x;++x){maximum=Max(maximum,Abs(int(a[y][x].r)-int(b[y][x].r)));maximum=Max(maximum,Abs(int(a[y][x].g)-int(b[y][x].g)));maximum=Max(maximum,Abs(int(a[y][x].b)-int(b[y][x].b)));}
            check(maximum<=2,U"settings indicator numerals average fonts alignment and glow match the gameplay renderer pixel for pixel");
            a.save(FsUtils::GetResourcePath(U"indicator-gameplay-{}-{}.png"_fmt(size.x,size.y)));b.save(FsUtils::GetResourcePath(U"indicator-settings-{}-{}.png"_fmt(size.x,size.y)));SkinPreset::Activate(hiddenSkin);
        }
        GaugePresentation::FixtureChartTime()=0;JudgeSettingOverlay overlay;
        check(overlay.focus()==5 && !overlay.session().menu,U"initial play mode remembers ALL Judge Offset");
        const auto a=ModMenuButtons::JudgeRect(),b=ModMenuButtons::VerticalRect(),c=ModMenuButtons::CustomRect();
        check(a.rightX()<b.x && b.rightX()<c.x && Abs((a.center().x+c.center().x)/2-size.x/2.0)<.01,U"three title icons are centered and disjoint");
        RenderTexture output{size,HasDepth::Yes};
        auto titleCanvas=noco::Canvas::LoadFromFile(FsUtils::GetResourcePath(U"mods/vertical/ui/title.noco"));
        TitleMenu titleMenu{TitleMenuItem::kStart,titleCanvas};
        for(int32 icon=0;icon<3;++icon){
            check(titleMenu.hoverAt(ModMenuButtons::IconRect(icon).center())&&titleMenu.focused(static_cast<TitleMenuItem>(icon)),U"main menu hover selects each mod icon");
            check(!titleMenu.hoverAt(ModMenuButtons::IconRect(icon).center()),U"stationary icon hover does not replay selection sound");
            titleCanvas->update(size,noco::HitTestEnabledYN::No);
            {const ScopedRenderTarget3D t3{output.clear(Palette::Black)};const ScopedRenderTarget2D t2{output};titleCanvas->draw();ModMenuButtons::DrawJudge(icon==0);ModMenuButtons::Draw(true,icon==1);ModMenuButtons::Draw(false,icon==2);Graphics2D::Flush();}
            Image image;output.readAsImage(image);image.save(FsUtils::GetResourcePath(U"judge-setting-main-icons-{}-{}-{}.png"_fmt(size.x,size.y,icon)));
        }
        for(int32 stage=0;stage<9;++stage){
            if(stage==1)overlay.requestMenu();
            if(stage==2)overlay.session().change(1,1);
            if(stage==3){overlay.session().change(5,1);overlay.requestExit();}
            if(stage==4){overlay.session().advance(.25);overlay.pointerDownAt(overlay.dialogRow(2).center());overlay.session().menu=false;overlay.session().advance(0);overlay.session().chip(JudgmentResult::kCritical,30);overlay.session().finishFrame();overlay.requestMenu();}
            if(stage==5){overlay.session().advance(.25);overlay.pointerDownAt(overlay.settingRow(1).center());}
            if(stage==6){overlay.session().change(0,-2);overlay.session().values.input=-10000;}
            if(stage==7||stage==8){auto& state=overlay.session();state.menu=false;if(stage==7)state.clearRecent();state.chip(JudgmentResult::kCritical,stage==7?-30:90);state.finishFrame();overlay.requestMenu();}
            overlay.update(1.0/60);
            if(stage==3)check(overlay.dialog()&&overlay.dirty(),U"unsaved exit opens a save decision dialog");
            {const ScopedRenderTarget3D t3{output.clear(Palette::Black)};const ScopedRenderTarget2D t2{output};overlay.draw();Graphics3D::Flush();Graphics2D::Flush();}
            Image image;output.readAsImage(image);image.save(FsUtils::GetResourcePath(U"judge-setting-{}-{}-{}.png"_fmt(size.x,size.y,stage)));
            overlay.layoutReport().save(FsUtils::GetResourcePath(U"judge-setting-{}-{}-{}.json"_fmt(size.x,size.y,stage)));
            check(!image.isEmpty(),U"native layout capture renders at supported resolution");
        }
        check(SkinPreset::Current()==hiddenSkin,U"forced indicators never alter the global indicator setting");
    }
#ifdef _WIN32
    const auto render=reinterpret_cast<DWORD(WINAPI*)(DWORD,float*)>(::GetProcAddress(::GetModuleHandleW(L"bassasio.dll"),"FakeAsioRender"));
    if(!render || !ksmaudio::IsASIOActive())throw Error{U"Judge replay requires mock ASIO"};
    for(const bool vertical:{false,true}){
        const Size size=vertical?Size{720,1280}:Size{1280,720};ConfigIni::SetBool(U"vertical_enabled",vertical);GameRender::Scope context{{size,vertical}};Window::Resize(size);Scene::Resize(size);
        ConfigIni::SetInt(ConfigIni::Key::kInputDelay,0);ConfigIni::SetInt(ConfigIni::Key::kVisualOffset,0);ConfigIni::SetInt(ConfigIni::Key::kLaserInputDelay,0);
        GaugePresentation::FixtureChartTime()=-3.4;JudgeSettingOverlay overlay;Vertical::DiagnosticInputReplay::Session input;
        const auto chart=kson::LoadKsonChartData(FsUtils::GetResourcePath(U"mods/enhanced/judge_setting/Preview_Piano.kson").toUTF8());const auto timing=kson::CreateTimingCache(chart.beat);
        const String mode=vertical?U"portrait":U"landscape";
        VideoWriter video{FsUtils::GetResourcePath(U"judge-setting-"+mode+U".mp4"),size,60};check(video.isOpen(),U"settings replay video opened at 60fps");
        RenderTexture output{size,HasDepth::Yes};std::vector<float> pcm(1470);uint64 countBeforeMenu=0;int frames=0;
        TextWriter trace{FsUtils::GetResourcePath(U"judge-setting-"+mode+U"-frames.csv")};trace.writeln(U"time,chartTime,menu,totalSamples,recentSamples,mean,last,loops");
        bool entered=false,returned=false;
        for(int tick=0;tick<2400;++tick){
            if(!System::Update())throw Error{U"Settings fixture closed"};
            render(735,pcm.data());const double elapsed=(tick+1)/60.0-3.4;const double time=elapsed<0?elapsed:std::fmod(elapsed,16.0);GaugePresentation::FixtureChartTime()=elapsed;
            uint32 held=0;
            if(!overlay.session().menu){
                for(size_t i=0;i<4;++i)for(const auto& [pulse,note]:chart.note.bt[i])if(Abs(time-kson::PulseToSec(pulse,chart.beat,timing))<1.0/120+.00001)held|=Vertical::DiagnosticInputReplay::Mask(kButtonBT_A+i);
                for(size_t i=0;i<2;++i)for(const auto& [pulse,note]:chart.note.fx[i])if(Abs(time-kson::PulseToSec(pulse,chart.beat,timing))<1.0/120+.00001)held|=Vertical::DiagnosticInputReplay::Mask(kButtonFX_L+i);
            }
            if(!entered && elapsed>=3.5){held=Vertical::DiagnosticInputReplay::Mask(kButtonFX_L)|Vertical::DiagnosticInputReplay::Mask(kButtonFX_R)|Vertical::DiagnosticInputReplay::Mask(kButtonStart);entered=true;countBeforeMenu=overlay.session().cumulative.count;}
            if(entered&&!returned&&elapsed>=6.5){held=Vertical::DiagnosticInputReplay::Mask(kButtonFX_L);returned=true;}
            std::array<double,2> laser{};for(size_t i=0;i<2;++i)for(const auto& [pulse,section]:chart.note.laser[i])if(Abs(time-kson::PulseToSec(pulse,chart.beat,timing))<1.0/120+.00001)laser[i]=i?-1:1;
            const bool wasMenu=overlay.session().menu;
            Vertical::DiagnosticInputReplay::Frame(held,laser);overlay.update(1.0/60);
            if(!wasMenu&&overlay.session().menu)countBeforeMenu=overlay.session().cumulative.count;
            if(elapsed>3.54&&elapsed<3.7)check(overlay.session().menu,U"FX+START chord cannot immediately resume play");
            if(elapsed>3.8&&elapsed<6.4)check(overlay.session().cumulative.count==countBeforeMenu,U"menu scrolling does not manufacture misses or samples");
            if(Abs(elapsed-6.5)<.001){
                check(!overlay.session().menu,U"first FX press resumes play");
                check(overlay.session().cumulative.count==countBeforeMenu+1&&overlay.session().recent.size()==1,U"resume press judges its actual FX chip exactly once");
            }
            if(elapsed>=0)trace.writeln(U"{},{},{},{},{},{},{},{}"_fmt(elapsed,overlay.game().previewTime(),overlay.session().menu,overlay.session().cumulative.count,overlay.session().recent.size(),overlay.session().recentMean().value_or(0),overlay.session().last.value_or(0),overlay.session().loops));
            if(elapsed>=2.5 && elapsed<8.5){
                {const ScopedRenderTarget3D t3{output.clear(Palette::Black)};const ScopedRenderTarget2D t2{output};overlay.draw();Graphics3D::Flush();Graphics2D::Flush();}
                Image image;output.readAsImage(image);check(video.writeFrame(image),U"native replay frame encoded");++frames;
            }
        }
        video.close();check(overlay.session().loops>=1,U"native fixed-clock replay crosses the 16-second boundary without ending settings");
        check(overlay.session().cumulative.count>countBeforeMenu+10,U"confirmed real inputs accumulate across loops");
        check(!overlay.finished()&&!overlay.game().playResult().isHardFailed,U"test stays playable without result or gauge failure");
        VideoReader reader{FsUtils::GetResourcePath(U"judge-setting-"+mode+U".mp4")};check(reader.isOpen()&&Abs(reader.getFPS()-60)<.01&&reader.getFrameCount()==static_cast<size_t>(frames),U"settings video reopens at native dimensions and frame count");
        Image frame;int decoded=0;while(reader.readFrame(frame))++decoded;check(decoded==frames,U"every settings video frame decodes");
    }
#endif
    // Drive the actual overlay actions rather than calling the persistence helper directly.
    {
        ConfigIni::SetBool(U"vertical_enabled",true);GameRender::Scope context{{Size{720,1280},true}};Window::Resize(720,1280);Scene::Resize(720,1280);GaugePresentation::FixtureChartTime()=0;
        JudgeSettingOverlay overlay;Vertical::DiagnosticInputReplay::Session input;
        const auto frame=[&](uint32 held,double delta=.05){Vertical::DiagnosticInputReplay::Frame(held);overlay.update(delta);};
        const auto press=[&](Button button){frame(Vertical::DiagnosticInputReplay::Mask(button));frame(0);};
        press(kButtonBack);check(overlay.session().menu&&!overlay.dialog()&&!overlay.finished(),U"ESC first enters menu without opening exit confirmation");
        frame(0,.25);press(kButtonBT_A);check(!overlay.session().menu&&!overlay.dialog(),U"BT resumes play from an unchanged menu");
        press(kButtonStart);check(overlay.session().menu&&!overlay.dialog(),U"START from play enters menu without activating the focused item");
        frame(0,.25);
        const auto valuesBeforeHover=overlay.session().values;
        overlay.pointerMoveAt(overlay.settingRow(3).center());check(overlay.focus()==3&&overlay.session().values==valuesBeforeHover,U"mouse hover selects the row without changing its value");
        overlay.pointerMoveAt(overlay.settingRow(5).center());check(overlay.focus()==5,U"mouse hover crosses between settings panels");
        check(overlay.visibleArrows(0)==std::pair{true,false},U"last FAST/SLOW choice hides right arrow");
        overlay.session().change(0,-2);check(overlay.visibleArrows(0)==std::pair{false,true},U"first FAST/SLOW choice hides left arrow");overlay.session().change(0,2);
        for(const int index:{5,6,7}){
            overlay.session().change(index,-10000);check(overlay.visibleArrows(index)==std::pair{false,true},U"minimum timing offset hides left arrow");
            overlay.session().change(index,20000);check(overlay.visibleArrows(index)==std::pair{true,false},U"maximum timing offset hides right arrow");overlay.session().change(index,-10000);
        }
        const ColorF base{Color{208,192,93}};
        check(JudgeSettingOverlay::inputColor(JudgeColor::kFastBlue,0)==base&&JudgeSettingOverlay::inputColor(JudgeColor::kFastBlue,9.9)==base,U"timing text starts at the requested neutral gold");
        for(const auto palette:{JudgeColor::kFastBlue,JudgeColor::kFastRed})for(const double ms:{-100.0,-30.0,-10.0,10.0,30.0,100.0}){
            const auto expected=base.lerp(ColorF{JudgeDirectionColor(palette,ms>0)},Min(1.0,Abs(ms)/100));
            check(Color{JudgeSettingOverlay::inputColor(palette,ms)}==Color{expected},U"timing text blends ten percent per ten milliseconds in the selected palette");
        }

        const double musicBefore=overlay.game().settingsMusicTime();
        overlay.session().change(6,35);frame(0,0);
        check(Abs(overlay.game().settingsMusicTime()-musicBefore)<.05&&Abs(overlay.game().previewTime()-.035)<1e-7,U"visual offset updates display immediately without seeking music");
        overlay.session().change(6,-35);frame(0,0);overlay.session().change(5,7);
        for(int i=0;i<3;++i)press(kButtonDown);
        check(overlay.focus()==8,U"direction keys navigate to SAVE");press(kButtonStart);
        check(!overlay.dirty()&&ConfigIni::GetInt(ConfigIni::Key::kInputDelay)==7,U"SAVE commits the live draft and updates saved baseline");
        overlay.session().change(5,2);overlay.requestExit();frame(0,.25);
#ifdef _WIN32
        const auto path=FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"config.ini");
        HANDLE locked=::CreateFileW(path.toWstr().c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        check(locked!=INVALID_HANDLE_VALUE,U"failure fixture locks replacement of its own config file");
        press(kButtonStart);
        check(overlay.dialog()&&!overlay.finished()&&overlay.dirty()&&ConfigIni::GetInt(ConfigIni::Key::kInputDelay)==7,U"failed save keeps dialog and draft without changing saved config");
        ::CloseHandle(locked);
#endif
        press(kButtonBT_A);check(!overlay.dialog()&&!overlay.session().menu,U"BT cancels exit dialog and resumes play");
        overlay.requestExit();frame(0,.25);press(kButtonDown);press(kButtonStart);
        check(overlay.finished()&&ConfigIni::GetInt(ConfigIni::Key::kInputDelay)==7,U"discard exits without saving later changes");
        JudgeSettingOverlay reentered;check(reentered.session().values.input==7,U"reentry reads the last successful save");
        reentered.pointerDownAt(Vec2{360,200});check(reentered.session().menu,U"window click enters menu controls");
        reentered.update(.25);
        const auto layout=reentered.layoutReport();const auto rect=layout[U"left"];
        // FAST/SLOW is last in the visual panel; use the public row geometry.
        const Vec2 point=reentered.settingRow(0).center()-Vec2{20,0};
        reentered.pointerDownAt(point);check(reentered.focus()==0&&reentered.session().values.mode==FastSlowMode::kOrigin,U"mouse hit-testing changes the intended setting");
        reentered.requestExit();reentered.update(.25);reentered.pointerDownAt(reentered.dialogRow(2).center());check(!reentered.dialog()&&!reentered.finished(),U"mouse cancels the unsaved exit popup");
        reentered.session().menu=false;reentered.pointerDownAt(reentered.closeButton().center());check(reentered.session().menu&&!reentered.dialog(),U"close button matches ESC from play by entering menu first");
        reentered.update(.25);reentered.pointerDownAt(reentered.closeButton().center());check(reentered.dialog()&&!reentered.finished(),U"close button matches ESC from menu by opening confirmation");
        reentered.update(.25);reentered.pointerDownAt(reentered.closeButton().center());check(!reentered.dialog()&&!reentered.finished(),U"close button matches ESC by cancelling the confirmation");
    }
    for(bool vertical:{false,true}){
        ConfigIni::SetBool(U"vertical_enabled",vertical);
        const Size size=vertical?Size{720,1280}:Size{1280,720};GameRender::Scope context{{size,vertical}};Window::Resize(size);Scene::Resize(size);
        ConfigIni::SetString(ConfigIni::Key::kHispeed,U"450");
        JudgeSettingOverlay overlay;Vertical::DiagnosticInputReplay::Session input;
        const auto frame=[&](uint32 held,std::array<double,2> laser={}){Vertical::DiagnosticInputReplay::Frame(held,laser);overlay.update(.05);};
        const auto start=Vertical::DiagnosticInputReplay::Mask(kButtonStart);
        frame(0);const int initial=overlay.game().settingsHispeed().value;
        frame(start);check(!overlay.session().menu,U"START press keeps lane play active until release");
        const int32 savedLaser=ConfigIni::GetInt(ConfigIni::Key::kLaserInputType,0);
        ConfigIni::SetInt(ConfigIni::Key::kLaserInputType,ConfigIni::Value::LaserInputType::kMouseXY);
        Vertical::DiagnosticInputReplay::state->laserDigital=false;
        for(int lane=0;lane<2;++lane){
            const int previous=overlay.game().settingsHispeed().value;
            std::array<double,2> laser{};laser[lane]=1;
            for(int tick=0;tick<4;++tick)frame(start,laser);
            check(!overlay.session().menu && overlay.game().settingsHispeed().value!=previous,U"each analog laser lane adjusts native hispeed while START is held");
        }
        ConfigIni::SetInt(ConfigIni::Key::kLaserInputType,ConfigIni::Value::LaserInputType::kKeyboard);
        Vertical::DiagnosticInputReplay::state->laserDigital=true;
        const int beforeDigital=overlay.game().settingsHispeed().value;
        frame(start|Vertical::DiagnosticInputReplay::Mask(kButtonRightLaserL));frame(start);
        check(overlay.game().settingsHispeed().value!=beforeDigital,U"configured digital laser uses the same native hispeed control");
        frame(0);check(overlay.session().menu,U"START release switches from hispeed adjustment to menu controls");
        for(int tick=0;tick<6;++tick)frame(0);
        const int focus=overlay.focus();const auto draft=overlay.session().values;const int speed=overlay.game().settingsHispeed().value;
        ConfigIni::SetInt(ConfigIni::Key::kLaserInputType,ConfigIni::Value::LaserInputType::kMouseXY);
        Vertical::DiagnosticInputReplay::state->laserDigital=false;
        frame(start);for(int tick=0;tick<4;++tick)frame(start,{1,-1});
        check(overlay.focus()==focus && overlay.session().values==draft && overlay.game().settingsHispeed().value==speed,U"START plus either knob in menu changes neither selected item nor draft nor hispeed");
        frame(0);frame(Vertical::DiagnosticInputReplay::Mask(kButtonBT_A));frame(0);
        check(!overlay.session().menu && overlay.game().settingsHispeed().value==speed,U"resuming lane controls retains the session hispeed");
        check(ConfigIni::GetString(ConfigIni::Key::kHispeed)==U"450",U"test hispeed does not overwrite user configuration implicitly");
        ConfigIni::SetInt(ConfigIni::Key::kLaserInputType,savedLaser);
    }
    {
        ConfigIni::SetBool(U"vertical_enabled",true);GameRender::Scope context{{Size{720,1280},true}};
        JudgeSettingOverlay clean;Vertical::DiagnosticInputReplay::Session input;
        Vertical::DiagnosticInputReplay::Frame(Vertical::DiagnosticInputReplay::Mask(kButtonBack));clean.update(.05);
        check(clean.session().menu&&!clean.finished(),U"unchanged BACK first enters menu from play");
        Vertical::DiagnosticInputReplay::Frame(0);clean.update(.25);
        Vertical::DiagnosticInputReplay::Frame(Vertical::DiagnosticInputReplay::Mask(kButtonBack));clean.update(.05);
        check(clean.finished()&&!clean.dialog(),U"unchanged exit returns directly without a save confirmation");
        JudgeSettingOverlay dirty;dirty.session().values.input+=1;dirty.requestExit();
        check(dirty.dialog()&&!dirty.finished(),U"unsaved changes still require save or discard confirmation");
    }
    const Array<String> expectedLanguages{U"English",U"Japanese",U"Korean",U"Simplified Chinese",U"Traditional Chinese"};
    const auto availableLanguages=I18n::GetAvailableLanguageList();
    check(availableLanguages.size()>=5,U"all five original languages remain available");
    for(size_t i=0;i<5;++i)check(availableLanguages[i]==expectedLanguages[i],U"option languages are ordered EN JP KR SC TC independently of file order");
    const String originalLanguage{ConfigIni::GetString(ConfigIni::Key::kLanguage)};
    const std::array<StringView,5> languages{U"English",U"Japanese",U"Korean",U"Simplified Chinese",U"Traditional Chinese"};
    for(size_t language=0;language<languages.size();++language){
        I18n::LoadLanguage(languages[language]);
        for(const auto key:magic_enum::enum_values<I18n::Mod>())check(!I18n::Get(key).isEmpty(),U"all mod localization keys resolve in every supported language");
        check(I18n::Get(I18n::Mod::NumberRange,U"test").includes(U"test"),U"translated format preserves its argument");
        for(const Size size:{Size{640,480},Size{1280,720},Size{1920,1080},Size{720,1280},Size{1080,1920}}){
            const bool vertical=size.y>size.x;ConfigIni::SetBool(U"vertical_enabled",vertical);GameRender::Scope context{{size,vertical}};Window::Resize(size);Scene::Resize(size);
            GaugePresentation::FixtureChartTime()=0;JudgeSettingOverlay overlay;overlay.requestMenu();overlay.update(.25);
            RenderTexture output{size,HasDepth::Yes};
            for(int stage=0;stage<2;++stage){
                if(stage){overlay.session().values.input+=1;overlay.requestExit();}
                {const ScopedRenderTarget3D t3{output.clear(Palette::Black)};const ScopedRenderTarget2D t2{output};overlay.draw();Graphics3D::Flush();Graphics2D::Flush();}
                Image image;output.readAsImage(image);image.save(FsUtils::GetResourcePath(U"judge-setting-l10n-{}-{}-{}-{}.png"_fmt(language,size.x,size.y,stage)));
            }
        }
    }
    const auto partialPath=FileSystem::PathAppend(I18n::GetDirectoryPath(),U"Mod Localization Partial.json");
    {TextWriter partial{partialPath};partial.writeln(U"{\"Mod\":{\"Select\":\"CUSTOM SELECT\"}}");}
    I18n::LoadLanguage(U"Mod Localization Partial");
    check(I18n::Get(I18n::Mod::Select)==U"CUSTOM SELECT"&&I18n::Get(I18n::Mod::SaveQuestion)==U"Save current settings?",U"old partial languages preserve translations and fall back to English mod keys");
    I18n::LoadLanguage(originalLanguage);FileSystem::Remove(partialPath);
    SkinPreset::Activate(originalSkin);
    report.writeln(U"TOTAL {} judge-setting checks passed"_fmt(checks));
}
}
