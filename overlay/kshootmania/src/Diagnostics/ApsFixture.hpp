#pragma once
#include "MusicGame/Judgment/JudgmentMain.hpp"
#include "MusicGame/Graphics/HUD/GaugePanel.hpp"
#include "Graphics/GameRenderContext.hpp"
#include "HighScore/KscIO.hpp"
#include "RuntimeConfig.hpp"
#include "Scenes/Select/BTOptionPanel.hpp"
#include "Vertical/DiagnosticInputReplay.hpp"
#include "Vertical/VerticalResources.hpp"
namespace ApsFixture {
inline void Run() {
    using namespace MusicGame; using namespace MusicGame::Judgment;
    if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Isolated APS fixture required"};
    TextWriter report{FsUtils::GetResourcePath(U"aps-test.txt")}; int checks=0;
    const auto check=[&](bool ok,StringView name){if(!ok)throw Error{String{name}}; ++checks;report.writeln(U"PASS "+String{name});};
    check(static_cast<bool>(PixelShaderAsset(U"ApsColorDodge")),U"Color Dodge shader compiles");
    for(double elapsed:{0.0,.2,.4,.5,.6,.7,.8,1.0,1.1,1.2}) {
        GaugePresentation presentation;presentation.update(4,true,false,false,0,0);
        auto visual=presentation.update(60,false,true,false,0,.1);
        visual=presentation.update(60,false,false,false,elapsed,.1+elapsed);
        const double animationTime=elapsed*GaugePresentation::kPlaybackSpeed;
        const double blend=GaugePresentation::EaseOut(animationTime);
        check(Abs(visual.percent-(4*(1-blend)+60*blend))<1e-8,U"1.5x playback applies to display interpolation");
        const double fade=Clamp((animationTime-GaugePresentation::FadeStart())/GaugePresentation::kFadeDuration,0.0,1.0);
        check(Abs(visual.bannerOpacity-(1-fade*fade))<1e-8,U"banner fade uses accelerated timeline");
        check(Abs(visual.dodge-GaugePresentation::EaseOut(animationTime/.7)*(1-.3*GaugePresentation::EaseOut((animationTime-.8)/.4)))<1e-8,U"image Color Dodge decreases to 70 percent and holds");
        if(elapsed<1.2)check(Abs(visual.burn-GaugePresentation::EaseOut((animationTime-.4)/.4))<1e-8,U"same-image Linear Burn darkens during 400-800ms");
    }
    {
        GaugePresentation p;p.update(3,true,false,false,0,0);p.update(20,false,true,false,0,.1);
        const auto a=p.update(40,false,false,false,.2,.3);const auto paused=p.update(40,false,false,true,10,.3);
        check(a.percent==paused.percent && a.bannerOpacity==paused.bannerOpacity,U"pause freezes all transition animation");
        const auto live=p.update(80,false,false,false,.3,.6);check(Abs(live.percent-(3*.0625+80*.9375))<1e-8,U"live NORMAL target follows later judgments during interpolation");
        const auto end=p.update(100,false,false,false,.2,.8);check(end.percent==100 && end.bannerOpacity>0 && end.dodge>0 && end.burn==1,U"gauge converges at accelerated speed while banner fade continues");
        const auto gone=p.update(100,false,false,false,.2,1.3);check(gone.bannerOpacity==0,U"banner is absent after accelerated fade");
        p.reset();check(p.update(100,true,false,false,0,0).bannerOpacity==1,U"retry restores protected banner");
        p.update(10,false,true,false,0,.1);check(p.update(70,false,false,false,0,-1).percent==70,U"rewind clears previous animation");
    }
    {
        Image swatches{4,1};swatches[0][0]=Color{0,0,0};swatches[0][1]=Color{64,96,128};swatches[0][2]=Color{160,192,224};swatches[0][3]=Color{255,255,255};
        Texture texture{swatches};RenderTexture output{Size{4,1}};const PixelShader shader=PixelShaderAsset(U"ApsColorDodge");
        for(const Float2 strength:{Float2{1,0},Float2{0,1},Float2{.5f,.75f}}){
            {const ScopedRenderTarget2D target{output.clear(Palette::Black)};const ScopedCustomShader2D effect{shader};const ScopedRenderStates2D sampler{SamplerState::ClampNearest};
                texture.draw(ColorF{strength.x,strength.y,0,1});Graphics2D::Flush();}
            Image pixels;output.readAsImage(pixels);bool matches=true;
            for(int x=0;x<4;++x){const auto source=swatches[0][x];const auto got=pixels[0][x];
                for(int c=0;c<3;++c){const double base=(c==0?source.r:c==1?source.g:source.b)/255.0;
                    const double dodge=base>=1?1:Min(1.0,base/(1-base));const double mid=base+(dodge-base)*strength.x;
                    const double expected=mid+(Max(0.0,mid+base-1)-(mid))*strength.y;
                    matches &= Abs((c==0?got.r:c==1?got.g:got.b)-expected*255)<2;
                }
            }
            check(matches,U"GPU Color Dodge and Linear Burn use original image colors including pure black");
        }
    }
    const int oldSelection=RuntimeConfig::GetGaugeSelection();
    for(int i=0;i<4;++i){RuntimeConfig::SetGaugeSelection(i);check(RuntimeConfig::GetGaugeSelection()==i && RuntimeConfig::GetApsEnabled()==(i==3) && static_cast<int>(RuntimeConfig::GetGaugeType())==Min(i,2),U"four menu choices preserve three compatible gauge types");}
    RuntimeConfig::SetGaugeSelection(oldSelection);
    for(const int maximum:{1,7,999,10000,77777,125000}) {
        ScoringStatus aps{1000000,maximum,GaugeType::kHardGauge,none,GameMode::kNormal,true};
        ScoringStatus normal{1000000,maximum,GaugeType::kNormalGauge,none,GameMode::kNormal};
        bool equal=true,switched=false; uint32 random=12345;
        for(int i=0;i<4000;++i){
            random=random*1664525+1013904223;const bool chip=(random&16)!=0;
            const auto event=(random%7==0)?JudgmentResult::kError:chip && random%3==0?JudgmentResult::kNearFast:chip && random%5==0?JudgmentResult::kNearSlow:JudgmentResult::kCritical;
            if(chip){aps.onChipOrLaserSlamJudgment(event);normal.onChipOrLaserSlamJudgment(event);}else{aps.onLongOrLaserLineJudgment(event);normal.onLongOrLaserLineJudgment(event);}
            equal &= aps.apsNormal().value==normal.gaugeValue() && aps.apsNormal().uncapped==normal.uncappedGaugeValue() && aps.score()==normal.score() && aps.totalJudgedCombo()==normal.totalJudgedCombo();
            if(i%17==0) switched |= aps.tryActivateAps();
            if(!aps.apsProtected())equal &= aps.gaugeValue()==normal.gaugeValue() && aps.gaugePercentageForGrade()==normal.gaugePercentageForGrade();
        }
        check(equal,U"4000 mixed chip slam long line judgments exactly match NORMAL including truncation and uncapped state");
        for(int i=0;i<40;++i)aps.onChipOrLaserSlamJudgment(JudgmentResult::kError);
        switched|=aps.tryActivateAps();check(switched && !aps.tryActivateAps() && !aps.apsProtected() && aps.effectiveGaugeType()==GaugeType::kNormalGauge,U"APS switches only once and cannot return to HARD");
    }
    {
        ScoringStatus aps{1000000,100000,GaugeType::kHardGauge,none,GameMode::kNormal,true};
        ScoringStatus normal{1000000,100000,GaugeType::kNormalGauge,none,GameMode::kNormal};
        for(int i=0;i<1000;++i){aps.onChipOrLaserSlamJudgment(JudgmentResult::kCritical);normal.onChipOrLaserSlamJudgment(JudgmentResult::kCritical);}
        for(int i=0;i<60;++i){aps.onLongOrLaserLineJudgment(JudgmentResult::kError);normal.onLongOrLaserLineJudgment(JudgmentResult::kError);}
        check(aps.tryActivateAps() && aps.gaugeValue()==normal.gaugeValue(),U"fallback preserves a nonzero hidden NORMAL accumulated before failure");
        for(int i=0;i<1000;++i){aps.onChipOrLaserSlamJudgment(JudgmentResult::kCritical);normal.onChipOrLaserSlamJudgment(JudgmentResult::kCritical);}
        check(aps.gaugePercentageInt(GaugeType::kNormalGauge)==100 && aps.gaugePercentageForGrade()==normal.gaugePercentageForGrade() && aps.score()==normal.score(),U"fallback recovers to NORMAL clear with matching grade and score");
        // Search the real integer calculation for a positive value below one percent.
        ScoringStatus low{1000000,100000,GaugeType::kHardGauge,none,GameMode::kNormal,true};bool found=false;
        for(int i=0;i<300 && !found;++i){low.onLongOrLaserLineJudgment(JudgmentResult::kError);if(low.gaugeValue()==0)low.onChipOrLaserSlamJudgment(JudgmentResult::kCritical);found=low.gaugeValue()>0 && low.gaugePercentageInt(GaugeType::kHardGauge)==0;}
        check(found && low.apsProtected() && low.tryActivateAps(),U"positive raw HARD below one percent uses original integer failure boundary");
        ScoringStatus frame{1000000,100000,GaugeType::kHardGauge,none,GameMode::kNormal,true};
        for(int i=0;i<40;++i)frame.onChipOrLaserSlamJudgment(JudgmentResult::kError);
        for(int i=0;i<10;++i)frame.onChipOrLaserSlamJudgment(JudgmentResult::kCritical);
        check(frame.apsProtected() && !frame.tryActivateAps(),U"same-frame recovery before checkpoint preserves original HARD frame semantics");
    }
    for(int frameSize:{1,6,64}) {
        kson::ChartData chart;chart.beat.bpm.emplace(0,120);chart.beat.timeSig.emplace(0,kson::TimeSig{});
        for(int i=0;i<400;++i)chart.note.bt[i%4][i*kson::kResolution4/4]=kson::Interval{};
        const auto timing=kson::CreateTimingCache(chart.beat);
        PlayOption option;option.gaugeType=GaugeType::kHardGauge;option.apsEnabled=true;
        // JudgmentMain constructs the actual judgment arrays; use its missed-note path below instead of synthetic input.
        JudgmentMain game{chart,timing,option,none,GameMode::kNormal};
        PlayOption normalOption=option;normalOption.gaugeType=GaugeType::kNormalGauge;normalOption.apsEnabled=false;
        JudgmentMain normal{chart,timing,normalOption,none,GameMode::kNormal};
        GameStatus status;ViewStatus view,normalView;bool switched=false,equal=true;
        const double step=frameSize*.125;
        for(double time=0;time<60+step;time+=step){
            status.currentTimeSec=status.currentTimeSecForButtonJudgment=status.currentTimeSecForLaserJudgment=time;
            status.currentPulse=status.currentPulseForButtonJudgment=status.currentPulseForLaserJudgment=kson::SecToPulse(time,chart.beat,timing);
            game.update(chart,status,view);normal.update(chart,status,normalView);
            switched|=game.tryActivateAps(view);
            if(switched)equal &= view.effectiveGaugeType==GaugeType::kNormalGauge && view.gaugePercentage==normalView.gaugePercentage && !view.apsProtected;
            equal &= view.score==normalView.score && view.exScore==normalView.exScore;
        }
        const auto result=game.playResult(chart,timing,60,IsHardFailedYN::No,false);
        check(switched && equal && result.playOption.gaugeType==GaugeType::kNormalGauge && result.achievement()==Achievement::kNone,U"actual missed chips transition at frame checkpoint and finish NORMAL without duplicated judgments at low FPS");
        check(RuntimeConfig::GetGaugeSelection()==oldSelection,U"fallback cannot change selected next-song gauge");
    }
    {
        PlayOption o;o.gaugeType=GaugeType::kHardGauge;o.apsEnabled=true;check(o.effectiveApsEnabled(),U"ordinary HARD APS enabled");
        o.gameMode=GameMode::kCourseMode;check(!o.effectiveApsEnabled(),U"course excludes APS");o.gameMode=GameMode::kNormal;
        o.skinPreview=true;check(!o.effectiveApsEnabled(),U"skin preview excludes APS");o.skinPreview=false;
        o.diagnosticSession=true;check(!o.effectiveApsEnabled(),U"diagnostics exclude APS");o.diagnosticSession=false;
        o.testPlayOption.emplace();check(!o.effectiveApsEnabled(),U"test play excludes APS");
        ScoringStatus plain{100000,100000,GaugeType::kHardGauge,none,GameMode::kNormal};
        for(int i=0;i<50;++i)plain.onChipOrLaserSlamJudgment(JudgmentResult::kError);
        check(!plain.tryActivateAps() && plain.effectiveGaugeType()==GaugeType::kHardGauge && plain.gaugePercentageInt(GaugeType::kHardGauge)==0,U"ordinary HARD retains original failure state");
        ScoringStatus retry{100000,100000,GaugeType::kHardGauge,none,GameMode::kNormal,true};
        check(retry.apsProtected() && retry.gaugePercentageInt(GaugeType::kHardGauge)==100 && retry.apsNormal().value==0,U"retry and new chart reset HARD 100 NORMAL zero");
        PlayResult result;result.playOption.gaugeType=GaugeType::kNormalGauge;result.totalCombo=100;result.comboStats.error=1;
        result.gaugePercentage=69.999;check(result.achievement()==Achievement::kNone,U"fallback below 70 fails");result.gaugePercentage=70;check(result.achievement()==Achievement::kCleared,U"fallback at 70 clears");
        result.playOption.gaugeType=GaugeType::kHardGauge;result.gaugePercentage=1;check(result.achievement()==Achievement::kCleared,U"surviving HARD uses original clear threshold");
    }
    {
        const auto canvas=noco::Canvas::LoadFromFile(Vertical::UIPath(U"ui/scene/select.noco"));
        if(!canvas)throw Error{U"APS menu canvas missing"};
        RuntimeConfig::SetGaugeSelection(0);BTOptionPanel panel{canvas};panel.loadFromConfigIni();
        Vertical::DiagnosticInputReplay::Session replay;
        const auto hold=Vertical::DiagnosticInputReplay::Mask(kButtonBT_B);
        const auto press=[&](Button key){Vertical::DiagnosticInputReplay::Frame(hold);panel.update(120);Vertical::DiagnosticInputReplay::Frame(hold|Vertical::DiagnosticInputReplay::Mask(key));panel.update(120);};
        for(int i=1;i<=3;++i){press(kButtonRight);check(RuntimeConfig::GetGaugeSelection()==i,U"real BT-B EFF RATE right input selects EASY NORMAL HARD HARD+APS");}
        press(kButtonRight);check(RuntimeConfig::GetGaugeSelection()==3,U"HARD+APS is right boundary without wrap");
        panel.saveToConfigIni();panel.hide();panel.loadFromConfigIni();check(RuntimeConfig::GetGaugeSelection()==3,U"closing and reopening options preserves HARD+APS");
        Vertical::DiagnosticInputReplay::Frame(hold);panel.update(120);canvas->update(noco::HitTestEnabledYN::No);
        RenderTexture output{Scene::Size()};{const ScopedRenderTarget2D target{output.clear(Palette::Black)};canvas->draw();Graphics2D::Flush();}
        Image image;output.readAsImage(image);image.save(FsUtils::GetResourcePath(U"aps-menu.png"));
        for(int i=2;i>=0;--i){press(kButtonLeft);check(RuntimeConfig::GetGaugeSelection()==i,U"real BT-B left input restores preceding gauge");}
        press(kButtonLeft);check(RuntimeConfig::GetGaugeSelection()==0,U"EASY is left boundary without wrap");
        RuntimeConfig::SetGaugeSelection(oldSelection);
    }
    const String player{ConfigIni::GetString(ConfigIni::Key::kCurrentPlayer)};ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,U"APS Fixture");
    const FilePath song=FileSystem::PathAppend(FsUtils::SongsDirectoryPath(),U"APS/test.ksh");FileSystem::CreateDirectories(FileSystem::ParentPath(song));{TextWriter w{song};w.writeln(U"title=APS\n--\n0000|00|--\n--");}
    PlayResult result;result.score=8000000;result.exScore=80;result.exMaximum=100;result.totalCombo=100;result.comboStats.error=1;result.gaugePercentage=75;result.gaugePercentageForGrade=75;result.playOption.apsEnabled=true;
    check(KscIO::WriteHighScoreInfo(song,result,KscKey{}),U"fallback writes original and additional NORMAL records");
    auto info=KscIO::ReadHighScoreInfo(song,KscKey{});check(info.medal()==Medal::kClear && info.playCount(GaugeType::kNormalGauge)==1,U"fallback awards only NORMAL lamp and counts once");
    result.playOption.gaugeType=GaugeType::kHardGauge;result.gaugePercentage=20;
    check(KscIO::WriteHighScoreInfo(song,result,KscKey{}.withGaugeType(GaugeType::kHardGauge)),U"survivor writes existing HARD record");
    result.playOption.gaugeType=GaugeType::kNormalGauge;result.gaugePercentage=0;result.exScore=90;
    check(KscIO::WriteHighScoreInfo(song,result,KscKey{}),U"later failed fallback saves NORMAL attempt");info=KscIO::ReadHighScoreInfo(song,KscKey{});
    check(info.medal()==Medal::kHardClear && info.playCount(GaugeType::kNormalGauge)==3 && info.exScore(GaugeType::kNormalGauge)==Optional<int64>{90},U"fallback preserves previous HARD clear and independent EX with one save per play");
    ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,player);
    const auto skin=SkinPreset::Current();const auto root=FsUtils::GetResourcePath(U"imgs");
    FileSystem::CreateDirectories(FileSystem::PathAppend(root,U"APS Complete"));FileSystem::CreateDirectories(FileSystem::PathAppend(root,U"APS Partial"));
    for(const auto name:{U"er_aps.png"}){
        FileSystem::Copy(FileSystem::PathAppend(root,U"default/"+String{name}),FileSystem::PathAppend(root,U"APS Complete/"+String{name}));
        SkinPreset::Activate({U"APS Complete",SkinPreset::MissingMode::Default});check(SkinPreset::Resolve(name).includes(U"APS Complete"),U"selected skin owns APS resource");
        SkinPreset::Activate({U"APS Partial",SkinPreset::MissingMode::Default});check(SkinPreset::Resolve(name).includes(U"default"),U"partial skin APS uses Default fallback");
        SkinPreset::Activate({U"APS Partial",SkinPreset::MissingMode::Error});check(Image{SkinPreset::Resolve(name)}[0][0]==Palette::Yellow,U"partial skin APS uses Yellow marker");
    }
    SkinPreset::Activate(skin);
    for(const Size size:{Size{640,480},Size{1280,720},Size{1920,1080},Size{720,1280},Size{1080,1920}}){
        const bool vertical=size.y>size.x;GameRender::Scope scope{{size,vertical}};
        MusicGame::Graphics::GaugePanel gauge{GaugeCalcType::kNormalHard};RenderTexture output{size};
        for(int state=0;state<8;++state){
            const double percent=state==0 || state==5?100:state==1 || state==6?30:state==2 || state==7?0:70;
            const auto type=state<3 || state>=5?GaugeType::kHardGauge:GaugeType::kNormalGauge;
            {const ScopedRenderTarget2D target{output.clear(ColorF{0,0})};if(vertical)gauge.drawVertical(percent,0,state<3,type);else gauge.draw(percent,0,state<3,type);Graphics2D::Flush();}
            Image image;output.readAsImage(image);check(!image.isEmpty(),U"native APS gauge render at target resolution");image.save(FsUtils::GetResourcePath(U"aps-{}-{}-{}.png"_fmt(size.x,size.y,state)));
        }
    }
    for(const Size size:{Size{640,480},Size{1280,720},Size{1920,1080},Size{720,1280},Size{1080,1920}}) {
        const bool vertical=size.y>size.x;GameRender::Scope scope{{size,vertical}};MusicGame::Graphics::GaugePanel gauge{GaugeCalcType::kNormalHard};RenderTexture output{size};
        int index=0;
        for(double elapsed:{0.0,.2,.4,.6,.7,.8,1.0,1.2}) {
            GaugePresentation p;p.update(4,true,false,false,0,0);p.update(60,false,true,false,0,.1);const auto visual=p.update(60,false,false,false,elapsed,.1+elapsed);
            {const ScopedRenderTarget2D target{output.clear(ColorF{0,0})};if(vertical)gauge.drawVertical(60,0,false,GaugeType::kNormalGauge,visual);else gauge.draw(60,0,false,GaugeType::kNormalGauge,visual);Graphics2D::Flush();}
            Image image;output.readAsImage(image);image.save(FsUtils::GetResourcePath(U"aps-timeline-{}-{}-{}.png"_fmt(size.x,size.y,index++)));
        }
    }
    report.writeln(U"TOTAL {} APS checks passed"_fmt(checks));
}
}
