#pragma once
#include "ksmaudio/AudioEffect/ParamController.hpp"
#include "Common/SkinPreset.hpp"
#include "MusicGame/Judgment/JudgmentMain.hpp"
#include "RuntimeConfig.hpp"
#include "Vertical/VerticalHUDLayout.hpp"
#include "MusicGame/Graphics/ViewportGeometry.hpp"
#include "MusicGame/Graphics/GraphicsDefines.hpp"
#include "MusicGame/Graphics/Highway/HighwayRenderTexture.hpp"

namespace SkinFixture
{
    inline bool Requested() { const auto& args=System::GetCommandLineArgs(); return args.contains(U"--skin-smoke-test") || args.contains(U"--skin-core-test"); }
    inline bool Enabled()
    {
        if (!Requested()) return false;
        if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"skin-test-fixture")))
            throw Error{U"Skin tests require an isolated App with skin-test-fixture marker"};
        return true;
    }
    inline bool Smoke() { return Enabled() && System::GetCommandLineArgs().contains(U"--skin-smoke-test"); }
    inline int& CheckCount(){static int value=0;return value;}
    inline void Check(bool condition, StringView description)
    {
        ++CheckCount();
        if (!condition) throw Error{U"Skin test failed: " + description};
    }
    inline void PreviewJudgmentChecks(){
        using namespace MusicGame;using namespace MusicGame::Judgment;
        const std::array<StringView,4> keys{ConfigIni::Key::kShowFastSlow,ConfigIni::Key::kTimingPosition,ConfigIni::Key::kTimingMsDisplay,ConfigIni::Key::kTimingMsColor};
        std::array<int32,4> saved;for(size_t i=0;i<keys.size();++i)saved[i]=ConfigIni::GetInt(keys[i],0);
        for(int mode=0;mode<3;++mode)for(int position=0;position<4;++position)for(int display=0;display<4;++display)for(int color=0;color<3;++color){
            ConfigIni::SetInt(keys[0],mode);ConfigIni::SetInt(keys[1],position);ConfigIni::SetInt(keys[2],display);ConfigIni::SetInt(keys[3],color);
            const auto loaded=RuntimeConfig::LoadTimingHudSettingsFromConfigIni();
            Check(static_cast<int>(RuntimeConfig::LoadFastSlowModeFromConfigIni())==mode && static_cast<int>(loaded.position)==position && static_cast<int>(loaded.display)==display && static_cast<int>(loaded.color)==color,U"preview and actual play share every timing HUD setting");
        }
        for(size_t i=0;i<keys.size();++i)ConfigIni::SetInt(keys[i],saved[i]);
        kson::ChartData chart;chart.beat.bpm.emplace(0,120);chart.beat.timeSig.emplace(0,kson::TimeSig{});
        const auto cache=kson::CreateTimingCache(chart.beat);
        std::array<kson::Pulse,14> pulses;
        const std::array<int32,14> lanes{0,4,5,1,2,3,4,5,0,1,2,3,4,5};
        for(int i=0;i<14;++i){
            pulses[i]=kson::SecToPulse(1.0+i*.5,chart.beat,cache);
            (lanes[i]<4?chart.note.bt[lanes[i]]:chart.note.fx[lanes[i]-4]).emplace(pulses[i],kson::Interval{});
        }
        chart.audio.keySound.fx.chipEvent["preview-fixture.wav"][0].emplace(pulses[1],kson::KeySoundInvokeFX{});
        PreviewChipSequence disabled{chart,cache,false};Check(disabled.size()==0,U"ordinary play has no demo schedule");
        PreviewChipSequence schedule{chart,cache,true};Check(schedule.size()==14,U"schedule includes chips only");
        for(int i=0;i<14;++i){const auto* e=schedule.nextDue(100);Check(e && e->pulse==pulses[i] && e->lane==lanes[i] && e->phase==i%7,U"chronological sequence independent of lane update order");}
        Check(!schedule.nextDue(100),U"consumed events never replay");
        PlayOption option;option.skinPreview=true;option.isAutoPlay=IsAutoPlayYN::Yes;option.fastSlowMode=FastSlowMode::kNew;
        JudgmentMain judgment{chart,cache,option,none,GameMode::kNormal};GameStatus game;ViewStatus view;
        const auto advance=[&](JudgmentMain& j,GameStatus& g,ViewStatus& v,double time){
            g.currentTimeSec=g.currentTimeSecForButtonJudgment=g.currentTimeSecForLaserJudgment=time;
            g.currentPulse=g.currentPulseForButtonJudgment=g.currentPulseForLaserJudgment=kson::SecToPulse(time,chart.beat,cache);j.update(chart,g,v);
        };
        advance(judgment,game,view,.9);Check(view.previewChipDemoCount==0 && view.inputTiming.count==0,U"no preview event before its musical time");
        for(int i=0;i<14;++i){
            const double time=kson::PulseToSec(pulses[i],chart.beat,cache)+1e-6;advance(judgment,game,view,time);
            Check(view.previewChipDemoCount==i+1,U"one demo phase per chip");
            const auto& lane=lanes[i]<4?game.btLaneStatus[lanes[i]]:game.fxLaneStatus[lanes[i]-4];
            Check(Abs(lane.keyBeamTimeSec-time)<1e-8,U"FAST and SLOW do not shift the note/effect time");
            Check((lane.keyBeamType==KeyBeamType::kPerfectCritical)==(i%7==3),U"P-CRITICAL effect only at its demo phase");
            Check(view.timingFeedback && view.timingFeedback->signedMs==kPreviewChipOutcomes[i%7].exampleMs,U"NEW+ receives the synthetic timing through normal feedback");
            Check(judgment.timingAdjustOffsetSec()==0,U"demo values never feed AutoSync");
            for(int mode=0;mode<6;++mode){
                const auto beam=SkinPreset::DisplayBeam(lane.keyBeamType,static_cast<SkinPreset::KeyBeamMode>(mode));
                const bool critical=i%7>=2 && i%7<=4,perfect=i%7==3;
                Check(static_cast<bool>(beam)==(mode!=0 && (mode!=4 || critical) && (mode!=5 || perfect)),U"all six key beam policies filter synthetic judgments");
            }
            const auto feedback=*view.timingFeedback;TimingHudSettings timing;timing.display=TimingMsDisplay::kAll;
            Check(!PresentTiming(timing,feedback,time).ms.isEmpty() && PresentTiming(timing,feedback,time+1.2).alpha==0,U"demo timing text uses shared display and fade");

            if(i==1)Check(view.detailedJudgments.fastNear==1,U"FX key sound does not promote the demo FAST NEAR");
            if(i==6)Check(view.inputTiming.count==5 && Abs(view.inputTiming.meanMs()+5)<1e-8 && Abs(view.inputTiming.absoluteMeanMs()-47)<1e-8,U"seven phases use only five successful timing samples");
        }
        const auto& d=view.detailedJudgments;
        Check(d.fastError==2 && d.fastNear==2 && d.fastCritical==2 && d.perfectCritical==2 && d.slowCritical==2 && d.slowNear==2 && d.slowError==2,U"fourteen chips increment each counter twice without AUTO duplicates");
        Check(view.inputTiming.count==10 && Abs(view.inputTiming.meanMs()+5)<1e-8,U"repeat cycle mean uses same success-only population");
        Check(view.score>0 && view.score<10000000 && view.displayCombo==0,U"demo judgments affect score and break combo on ERROR");
        const auto stats=view.inputTiming;advance(judgment,game,view,10);advance(judgment,game,view,10);
        Check(view.previewChipDemoCount==14 && view.inputTiming.count==stats.count,U"same-frame updates cannot double count");
        JudgmentMain catchup{chart,cache,option,none,GameMode::kNormal};GameStatus caught;ViewStatus caughtView;advance(catchup,caught,caughtView,10);
        Check(caughtView.detailedJudgments.fastError==2 && caughtView.detailedJudgments.fastNear==2 && caughtView.detailedJudgments.fastCritical==2 && caughtView.detailedJudgments.perfectCritical==2 && caughtView.detailedJudgments.slowCritical==2 && caughtView.detailedJudgments.slowNear==2 && caughtView.detailedJudgments.slowError==2 && caughtView.inputTiming.count==10,U"low FPS catches all global events exactly once");
        auto normal=option;normal.skinPreview=false;JudgmentMain autoplay{chart,cache,normal,none,GameMode::kNormal};GameStatus autoGame;ViewStatus autoView;advance(autoplay,autoGame,autoView,10);
        Check(autoView.previewChipDemoCount==0 && autoView.detailedJudgments.perfectCritical==14 && autoView.inputTiming.count==0,U"ordinary AUTO remains unmeasured P-CRITICAL");
        auto simultaneous=chart;for(auto& lane:simultaneous.note.bt)lane.clear();for(auto& lane:simultaneous.note.fx)lane.clear();
        for(int i=3;i>=0;--i)simultaneous.note.bt[i].emplace(pulses[0],kson::Interval{});
        for(int i=1;i>=0;--i)simultaneous.note.fx[i].emplace(pulses[0],kson::Interval{});
        simultaneous.note.bt[0].emplace(pulses[1],kson::Interval{240});
        PreviewChipSequence chord{simultaneous,cache,true};Check(chord.size()==6,U"long note excluded from sequence");
        for(int i=0;i<6;++i){const auto* e=chord.nextDue(100);Check(e && e->lane==i && e->phase==i,U"simultaneous tie order BT A-D then FX L-R");}
        PreviewChipSequence reset{chart,cache,true};Check(reset.processed()==0 && reset.nextDue(100)->phase==0,U"new preview loop starts at FAST ERROR");
    }
    inline void ViewportChecks()
    {
        using namespace SkinPreset;
        using namespace MusicGame::Graphics;
        const auto original=Current();
        PreviewJudgmentChecks();
        using namespace ksmaudio::AudioEffect;
        ParamValueSetDict base{{ParamID::kMix,ValueSet{.off=.1f}}};
        std::unordered_map<ParamID,std::map<float,ValueSet>> timeline{{ParamID::kMix,{{1.0f,ValueSet{.off=.2f}},{4.0f,ValueSet{.off=.4f}}}}};
        ParamController params{base,timeline};
        for(int cycle=0;cycle<20;++cycle){
            params.rewind(0);Check(Abs(params.currentParams().at(ParamID::kMix).off-.1f)<1e-6,U"rewind restores base parameter");
            params.update(2);Check(Abs(params.currentParams().at(ParamID::kMix).off-.2f)<1e-6,U"param_change reapplies on every loop");
            params.setOverrideParams({{ParamID::kMix,ValueSet{.off=.9f}}});params.update(6);
            Check(Abs(params.currentParams().at(ParamID::kMix).off-.9f)<1e-6,U"active override wins");
            params.rewind(2);Check(Abs(params.currentParams().at(ParamID::kMix).off-.2f)<1e-6,U"rewind clears override and restores target timeline");
        }
        const auto path=FsUtils::GetResourcePath(U"skin_preset.json");
        JSON legacy;
        legacy[U"folder"]=U"default";
        legacy[U"missing"]=U"default";
        Check(legacy.save(path),U"legacy fixture save");
        Initialize();
        Check(Current().background1Brightness==100 && Current().background2Opacity==100 && Current().laserWidthPercent==100 && Current().noteHeightPercent==100,U"legacy settings default to 100 percent");
        Check(Current().indicatorPortrait==IndicatorPortrait::Bottom && Current().indicatorLandscape && Current().keyBeamMode==KeyBeamMode::Normal,U"legacy presentation defaults");
        using Beam=MusicGame::Judgment::KeyBeamType;
        for(int source=0;source<static_cast<int>(Beam::kNumBeamTypes);++source){
            const Beam b=static_cast<Beam>(source);
            Check(!DisplayBeam(b,KeyBeamMode::Off),U"Off hides every key beam");
            Check(DisplayBeam(b,KeyBeamMode::Normal)==Optional<Beam>{b},U"Normal preserves original beam");
            const bool success=b==Beam::kNear || b==Beam::kNearSlow || b==Beam::kNearFast || b==Beam::kCritical || b==Beam::kPerfectCritical;
            Check(DisplayBeam(b,KeyBeamMode::AllCrit)==Optional<Beam>{success?Beam::kCritical:b},U"All Crit unifies beam");
            Check(DisplayBeam(b,KeyBeamMode::AllPerfect)==Optional<Beam>{success?Beam::kPerfectCritical:b},U"All P-Crit unifies beam");
            const auto crit=DisplayBeam(b,KeyBeamMode::OnlyCrit),perfect=DisplayBeam(b,KeyBeamMode::OnlyPerfect);
            Check(static_cast<bool>(crit)==(b==Beam::kCritical || b==Beam::kPerfectCritical),U"Only Crit filters noncritical input");
            Check(!crit || *crit==Beam::kCritical,U"Only Crit uses ordinary critical texture");
            Check(static_cast<bool>(perfect)==(b==Beam::kPerfectCritical),U"Only P-Crit filters ordinary critical and other input");
        }
        Settings newOptions;newOptions.indicatorPortrait=IndicatorPortrait::Both;newOptions.indicatorLandscape=false;newOptions.keyBeamMode=KeyBeamMode::OnlyPerfect;
        Check(Save(newOptions),U"presentation settings save");Initialize();Check(Current()==newOptions,U"presentation settings roundtrip");
        Settings invalid;
        invalid.background1Brightness=-50; invalid.background2Opacity=500;
        invalid.indicatorPortrait=static_cast<IndicatorPortrait>(99);invalid.keyBeamMode=static_cast<KeyBeamMode>(-1);
        invalid.laserWidthPercent=999; invalid.noteHeightPercent=-10;
        Check(Save(invalid),U"sanitize save");
        Initialize();
        Check(Current().background1Brightness==0 && Current().background2Opacity==100 && Current().laserWidthPercent==125 && Current().noteHeightPercent==50,U"numeric limits");
        Check(Current().indicatorPortrait==IndicatorPortrait::Both && Current().keyBeamMode==KeyBeamMode::Off,U"invalid presentation settings clamped");
        Check(StepLaserPercent(100,1)==105 && StepLaserPercent(100,-1)==95,U"laser five percent steps");
        Check(Abs(StepLaserPercent(70,1)-kVerticalLaserPercent)<1e-8 && Abs(StepLaserPercent(75,-1)-kVerticalLaserPercent)<1e-8 && StepLaserPercent(kVerticalLaserPercent,1)==75 && StepLaserPercent(kVerticalLaserPercent,-1)==70,U"exact vertical detent");
        Check(StepLaserPercent(50,-1)==50 && StepLaserPercent(120,1)==125 && StepLaserPercent(125,1)==125,U"laser endpoints clamp");
        Activate(Settings{});
        const Texture before=TextureAsset(U"bt_chip.gif");
        const auto textureId=before.id();
        HighwayRenderTexture plane;
        plane.drawBaseTexture(0.0);
        Graphics2D::Flush();
        Image reference;
        plane.additiveTexture().readAsImage(reference);
        Check(reference.width()==kHighwayTextureSizeWide.x,U"padded render target");
        for(const double percent : {50.0,kVerticalLaserPercent,100.0,125.0})
        {
            Settings value; value.laserWidthPercent=percent;
            Activate(value);
            Check(TextureAsset(U"bt_chip.gif").id()==textureId,U"numeric update does not reload textures");
            Check(ViewportGeometry::RailX(ViewportGeometry::CoreLeft())==ViewportGeometry::CoreLeft() && ViewportGeometry::RailX(256-ViewportGeometry::CoreLeft())==256-ViewportGeometry::CoreLeft(),U"rail inner edges fixed");
            Check(Abs(ViewportGeometry::RailX(0)-(ViewportGeometry::CoreLeft()*(1-percent/100/(GameRender::VerticalEnabled()?8.0/11:1))))<1e-8,U"left rail expands outward");
            const double minX=kLaserPositionOffsetWide.x+ViewportGeometry::LaserCenter(0,2)-ViewportGeometry::LaserWidth()/2;
            const double maxX=kLaserPositionOffsetWide.x+ViewportGeometry::LaserCenter(1,2)+ViewportGeometry::LaserWidth()/2;
            Check(minX>=0 && maxX<=kHighwayTextureSizeWide.x,U"125 percent wide laser fits render target");
            plane.drawBaseTexture(0.0);
            Graphics2D::Flush();
            Image actual;
            plane.additiveTexture().readAsImage(actual);
            for(const int y : {100,500,900}) for(int x=static_cast<int>(ViewportGeometry::CoreLeft());x<256-ViewportGeometry::CoreLeft();++x)
                Check(actual[y][kHighwayTextureOffsetX+x]==reference[y][kHighwayTextureOffsetX+x],U"central lane pixels remain unchanged");
        }
        {
            const GameRender::Scope portrait{GameRender::Context{Size{720,1280},true}};
            Settings native;native.laserWidthPercent=kVerticalLaserPercent;Activate(native);
            Check(Abs(ViewportGeometry::LaserWidth()-48.0*32/44)<1e-8 && Abs(ViewportGeometry::LaserSpan()-(256-48.0*32/44))<1e-8,U"72.73 percent restores original portrait laser width and centers");
            Check(Abs(ViewportGeometry::RailX(0))<1e-8 && Abs(ViewportGeometry::RailX(256)-256)<1e-8,U"72.73 percent restores original portrait rail boundaries");
            Settings common;Activate(common);
            Check(Abs(ViewportGeometry::LaserWidth()-48)<1e-8 && Abs(ViewportGeometry::LaserSpan()-232)<1e-8,U"portrait 100 percent uses landscape reference width and portrait core");
            Check(Abs(ViewportGeometry::RailX(0)+12)<1e-8 && Abs(ViewportGeometry::RailX(256)-268)<1e-8,U"portrait 100 percent expands rails outward without changing button core");
            const GameRender::Scope landscape{GameRender::Context{Size{1280,720},false}};
            Check(Abs(ViewportGeometry::LaserWidth()-48)<1e-8 && Abs(ViewportGeometry::LaserSpan()-208)<1e-8,U"landscape 100 percent remains unchanged");
        }
        for(const int percent : {50,100,200})
        {
            Settings value; value.noteHeightPercent=percent; Activate(value);
            Check(Abs(ViewportGeometry::NoteTop(100,40)+0.8*40*ViewportGeometry::NoteScale()-132)<1e-8,U"note 80 percent pivot stays fixed");
        }
        Settings custom;
        custom.background1Brightness=75;custom.background2Opacity=40;custom.laserWidthPercent=kVerticalLaserPercent;custom.noteHeightPercent=125;
        Check(Save(custom),U"viewport settings save");
        Initialize();
        Check(Current()==custom,U"all viewport values restore");
        custom.folder=U"Alpha";Activate(custom);
        Check(Current().background1Brightness==75 && Current().noteHeightPercent==125,U"viewport values are global across skins");
        Activate(original);
        Check(Save(original),U"restore viewport fixture");
    }
    inline void Core()
    {
        if (!Enabled()) return;
        using namespace SkinPreset;
        const auto original=Current();
        const auto entries=Discover();
        Check(!entries.isEmpty() && entries.front().folder==U"default",U"default pinned first");
        Check(entries.size()>=5 && entries[1].folder==U"Alpha" && entries[2].folder==U"beta" && entries[3].folder==U"z-last",U"case insensitive folder ordering");
        Check(entries.any([](const Entry& e){return e.folder==U"한글 Partial" && e.author==U"테스트 제작자";}),U"Unicode metadata");
        for(const String base:{U"result_v2_judge.png",U"play_judge_indicator.png",U"play_judge_indicator_minimum.png"})for(const auto palette:{JudgeColor::kFastBlue,JudgeColor::kFastRed}){
            const auto name=MusicGame::JudgeImage(base,palette);
            Activate({U"Alpha",MissingMode::Default});Check(Resolve(name).includes(U"Alpha"),U"complete skin variant wins");
            Activate({U"한글 Partial",MissingMode::Default});Check(Resolve(name).includes(U"default"),U"missing variant falls back to same-color default");
            Activate({U"한글 Partial",MissingMode::Error});const Image missing{Resolve(name)};Check(missing[0][0]==Palette::Yellow,U"missing variant obeys Yellow policy");
        }
        Activate({U"한글 Partial",MissingMode::Default});
        Check(Resolve(U"score_header.png").includes(U"한글 Partial"),U"selected file");
        Check(Resolve(U"imgs/bg.jpg").includes(U"default"),U"missing file fallback");
        Check(Resolve(U"standby.jpg").includes(U"default"),U"corrupt file fallback");
        const auto physical=Resolve(U"score_header.png");
        Check(!noco::Asset::GetOrLoadTexture(physical,noco::MipmapEnabledYN::No).isEmpty(),U"physical image paths are not resolved twice");
        const auto baseInfo=ImageDecoder::GetImageInfo(Resolve(U"bt_chip.gif"));
        Activate({U"한글 Partial",MissingMode::Error});
        const Image yellow{Resolve(U"bt_chip.gif")};
        Check(baseInfo && yellow.size()==baseInfo->size && yellow[0][0]==Palette::Yellow,U"yellow placeholder geometry and pixels");
        const auto& ui=noco::Asset::GetOrLoadTexture(U"imgs/bt_chip.gif",noco::MipmapEnabledYN::No);
        Check(ui.size()==yellow.size(),U"UI loader resolves migrated logical path");
        Check(Save(Current()),U"initial save");
        Check(Save(original),U"replace existing settings");
        const auto temp=FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"skin_preset.json.tmp");
        FileSystem::CreateDirectories(temp);
        Check(!Save({U"Alpha",MissingMode::Error}),U"save failure is reported");
        FileSystem::Remove(temp);
        Initialize();
        Check(Current()==original,U"reload saved selection");
        Check(Save({U"deleted-folder",MissingMode::Default}),U"save stale selection fixture");
        Initialize();
        Check(Current().folder==U"default",U"deleted folder recovery");
        Activate(original);
        Check(Save(original),U"restore fixture settings");
        // A preview must keep its own HUD coordinates even inside a larger host window.
        const auto hostContext=GameRender::Current();
        {
            const GameRender::Scope preview{GameRender::Context{Size{720,1280},true}};
            const auto progress=Vertical::HUDRect(U"Progress");
            Check(Abs(progress.x-34)<1e-8 && Abs(progress.y-169)<1e-8 && Abs(progress.w-655)<1e-8 && Abs(progress.h-10)<1e-8,U"720 preview progress stays between title and song info regardless of host size");
            {
                const GameRender::Scope larger{GameRender::Context{Size{1080,1920},true}};
                const auto large=Vertical::HUDRect(U"Progress");
                Check(Abs(large.y-253.5)<1e-8 && Abs(large.w-982.5)<1e-8,U"native 1080 HUD scales once");
            }
            Check(Abs(Vertical::HUDRect(U"Progress").y-169)<1e-8,U"preview HUD restored after nested render context");
        }
        Check(GameRender::Size()==hostContext.outputSize,U"HUD test restores host render context");
        ViewportChecks();
        TextWriter{FsUtils::GetResourcePath(U"core-passed.txt")}.writeln(U"TOTAL {} core checks passed"_fmt(CheckCount()));
    }
}
