#pragma once
#include "MusicGame/TimingFeedback.hpp"
#include "RuntimeConfig.hpp"
#include "MusicGame/Judgment/JudgmentMain.hpp"
#include "Ini/ConfigIni.hpp"
#include "Input/KeyConfig.hpp"
#include "Common/FsUtils.hpp"
namespace TimingHudFixture {
inline void Run() {
    using namespace MusicGame;
    using namespace MusicGame::Judgment;
    if (!FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Isolated fixture required"};
    TextWriter report{FsUtils::GetResourcePath(U"timing-hud-test.txt")};
    size_t checks=0;
    const auto check=[&](bool condition,StringView text) {
        if (!condition) throw Error{String{text}};
        ++checks;report.writeln(U"PASS "+String{text});
    };
    {
        InputTimingStats input;
        check(!input.roundedMeanMs() && input.meanMs()==0 && input.absoluteMeanMs()==0,U"live timing starts empty without a fabricated result average");
        input.add(JudgmentResult::kCritical,15.5);input.add(JudgmentResult::kNearSlow,-9.9);
        check(input.count==2 && Abs(input.meanMs()-2.8)<1e-10 && Abs(input.absoluteMeanMs()-12.7)<1e-10,U"signed and absolute means use the same successful real inputs");
        check(input.roundedMeanMs()==Optional<int32>{3},U"original result integer average rounds the unrounded live value");
        input.add(JudgmentResult::kError,100);input.add(JudgmentResult::kError,-100);
        input.add(JudgmentResult::kCritical);input.add(JudgmentResult::kUnspecified,1);
        input.add(JudgmentResult::kCritical,std::numeric_limits<double>::infinity());
        input.add(JudgmentResult::kNearFast,std::numeric_limits<double>::quiet_NaN());
        check(input.count==2,U"ERROR, unmeasured AUTO and nonfinite values do not alter original average population");
        InputTimingStats boundary;
        boundary.add(JudgmentResult::kCritical,0);boundary.add(JudgmentResult::kNearFast,150);
        boundary.add(JudgmentResult::kNearSlow,-150);boundary.add(JudgmentResult::kCritical,151);
        check(boundary.count==4 && boundary.sumMs==151 && boundary.absoluteSumMs==451,U"original averaging has no new 150ms cutoff and includes measured zero");
    }
    static_assert(static_cast<int>(FastSlowMode::kHide)==0 && static_cast<int>(FastSlowMode::kOrigin)==1 && static_cast<int>(FastSlowMode::kNew)==2);
    TimingHudSettings settings;
    for (double ms : {0.0,20.9,-20.9,21.0,-21.0,21.04,-21.04,21.1,-21.1}) {
        const auto view=PresentTiming(settings,{JudgmentResult::kCritical,ms,1},1);
        check(!view.word && bool(view.ms)==(std::abs(ms)>21),U"CRITICAL ms filter uses unrounded strictly outside twenty-one-ms boundary");
        check((view.color==Color{178,178,178})==(std::abs(ms)<=21),U"outside twenty-one-ms colors honor sign and exclude boundary");
    }
    settings.display=TimingMsDisplay::kAll;
    settings.color=TimingMsColor::kOff;
    for (auto result : {JudgmentResult::kCritical,JudgmentResult::kNearFast,JudgmentResult::kNearSlow,JudgmentResult::kError}) {
        const double ms=result==JudgmentResult::kNearSlow ? -60.3 : 60.3;
        const auto view=PresentTiming(settings,{result,ms,1},1);
        check(bool(view.word)==(result!=JudgmentResult::kCritical) && bool(view.ms),U"ALL includes measured ERROR and words exclude CRITICAL");
        check(view.color==Color{178,178,178},U"color OFF makes ms neutral independently of directional text");
    }
    settings.display=TimingMsDisplay::kNear;settings.color=TimingMsColor::kNear;
    for(auto result:{JudgmentResult::kCritical,JudgmentResult::kNearFast,JudgmentResult::kNearSlow,JudgmentResult::kError}) {
        const auto view=PresentTiming(settings,{result,-60.3,1},1);
        check(bool(view.ms),U"NEAR display includes every measured error at or beyond the NEAR boundary");
        check(view.color==JudgeDirectionColor(JudgeColor::kFastBlue,false),U"NEAR colors include large errors and promoted FX timing");
    }
    // Cross-product tests exercise unrounded thresholds independently of final score judgment.
    for(auto palette:{JudgeColor::kFastBlue,JudgeColor::kFastRed})
    for(auto result:{JudgmentResult::kCritical,JudgmentResult::kNearFast,JudgmentResult::kNearSlow,JudgmentResult::kError})
    for(double magnitude:{0.0,11.999,12.0,12.001,15.999,16.0,16.001,20.999,21.0,21.001,43.999,44.0,44.001,114.0,150.0})
    for(double sign:{-1.0,1.0})
    for(int display=0;display<4;++display)
    for(int color=0;color<4;++color){
        TimingHudSettings style;style.judgeColor=palette;style.display=TimingDisplayChoice(display);style.color=static_cast<TimingMsColor>(color);
        const auto view=PresentTiming(style,{result,magnitude*sign,1},1);
        const bool show=display==3 || (display==1 && magnitude>=44) || (display==2 && magnitude>21);
        const bool tint=(color==1 && magnitude>=44) || (color==2 && magnitude>21) || (color==3 && magnitude>16);
        check(bool(view.ms)==show,U"OFF NEAR CRIT ALL numeric filter boundaries are independent of final judgment");
        check(view.color==(tint?JudgeDirectionColor(palette,sign>0):Color{178,178,178}),U"OFF NEAR CRIT sixteen-ms color boundaries preserve both palettes");
    }
    for(int cursor=0;cursor<4;++cursor)check(TimingDisplayCursor(TimingDisplayChoice(cursor))==cursor,U"display menu order round trips without changing persisted IDs");
    static_assert(static_cast<int>(TimingMsDisplay::kAll)==1 && static_cast<int>(TimingMsDisplay::kNear)==2 && static_cast<int>(TimingMsDisplay::kCrit)==3);
    settings.display=TimingMsDisplay::kOff;
    check(PresentTiming(settings,{JudgmentResult::kNearFast,60,1},1).word==U"FAST"
        && !PresentTiming(settings,{JudgmentResult::kNearFast,60,1},1).ms,U"ms OFF retains directional word");
    settings.display=TimingMsDisplay::kAll;
    check(PresentTiming(settings,{JudgmentResult::kCritical,-0.01,1},1).ms==U"0.0ms",U"negative zero is normalized");
    check(PresentTiming(settings,{JudgmentResult::kNearSlow,-47.8,1},1).ms==U"-47.8ms",U"SLOW uses minus and lowercase ms");
    check(PresentTiming(settings,{JudgmentResult::kNearFast,12.3,1},1).ms==U"+12.3ms",U"FAST uses plus and lowercase ms");
    check(PresentTiming(settings,{JudgmentResult::kCritical,0,1},1.84).alpha>0.89,U"full opacity lasts 840ms");
    const auto fading=PresentTiming(settings,{JudgmentResult::kCritical,0,1},2.02).alpha;
    check(fading>0.44 && fading<0.46,U"last 360ms fade without movement");
    check(PresentTiming(settings,{JudgmentResult::kCritical,0,1},2.2).alpha==0,U"display expires after 1200ms");
    check(PresentTiming(settings,{JudgmentResult::kCritical,0,1},0.9).alpha==0,U"backward time never revives a future display");
    TimingFeedbackState state;
    state.offer(JudgmentResult::kCritical,12);state.offer(JudgmentResult::kNearSlow,-60);state.offer(JudgmentResult::kNearFast,60);
    auto event=state.commit(1);
    check(event && event->signedMs==-60 && event->result==JudgmentResult::kNearSlow,U"simultaneous inputs select largest error with stable first-lane tie");
    state.offer(JudgmentResult::kCritical,5);event=state.commit(2);
    check(event && event->signedMs==5 && event->displayedAt==2,U"new frame replaces all previous display fields");
    state.offer(JudgmentResult::kError,none);state.offer(JudgmentResult::kCritical,none);
    check(state.commit(3)->displayedAt==2,U"missed and automatic judgments do not fabricate measured timing");
    state.clear();check(!state.commit(4),U"pause seek restart exit clear retained and pending feedback");
    {
        TimingFeedbackState words;
        TimingHudSettings style;style.display=TimingMsDisplay::kAll;style.color=TimingMsColor::kOff;
        words.offer(JudgmentResult::kNearFast,60);words.commit(1);
        words.offer(JudgmentResult::kCritical,-5);auto latest=words.commit(1.1);
        auto view=PresentTiming(style,*latest,1.2);
        check(view.word==U"FAST" && view.wordAlpha>0.89 && view.ms==U"-5.0ms",U"CRITICAL updates ms without clearing retained FAST word");
        check(view.wordColor==JudgeDirectionColor(JudgeColor::kFastBlue,true) && view.color==Color{178,178,178},U"FAST is always colored while ms color is OFF");
        words.offer(JudgmentResult::kCritical,8);latest=words.commit(2.4);
        view=PresentTiming(style,*latest,2.5);
        check(!view.word && view.alpha>0,U"repeated CRITICAL does not extend the word lifetime");
        words.offer(JudgmentResult::kNearSlow,-60);latest=words.commit(2.6);
        view=PresentTiming(style,*latest,2.7);
        check(view.word==U"SLOW" && view.wordColor==JudgeDirectionColor(JudgeColor::kFastBlue,false),U"new NEAR replaces direction and restarts word timer");
        for(auto color:{TimingMsColor::kOff,TimingMsColor::kNear,TimingMsColor::kCrit}) {
            style.color=color;view=PresentTiming(style,*latest,2.7);
            check(view.wordColor==JudgeDirectionColor(JudgeColor::kFastBlue,false),U"SLOW color never depends on the ms color preset");
        }
        words.clear();words.offer(JudgmentResult::kCritical,5);latest=words.commit(3);
        check(!PresentTiming(style,*latest,3).word,U"clearing state also removes retained word across pause or seek");
    }

    for(int cursor=0;cursor<9;++cursor) {
        check(TimingMenuCursor(cursor,1,false)==Max(cursor,4) && TimingMenuCursor(cursor,-1,false)==Max(cursor,4),U"hidden NEW+ rows cannot retain focus");
        check(TimingMenuCursor(cursor,1,true)==cursor,U"NEW+ enables all nine rows");
    }
    check(TimingMenuCursor(4-1,-1,true)==3 && TimingMenuCursor(3+1,1,true)==4,U"ASSIST TICK and TIMING POSITION are adjacent");
    for(auto palette:{JudgeColor::kFastBlue,JudgeColor::kFastRed}) {
        TimingHudSettings style;style.judgeColor=palette;style.color=TimingMsColor::kNear;style.display=TimingMsDisplay::kAll;
        const auto fast=PresentTiming(style,{JudgmentResult::kNearFast,60,1},1);
        const auto slow=PresentTiming(style,{JudgmentResult::kNearSlow,-60,1},1);
        check(fast.wordColor==JudgeDirectionColor(palette,true) && fast.color==fast.wordColor,U"FAST text and ms share selected palette");
        check(slow.wordColor==JudgeDirectionColor(palette,false) && slow.color==slow.wordColor,U"SLOW text and ms share selected palette");
        check(JudgeTimingColor(palette,0)==Color{178,178,178},U"zero average remains neutral in both palettes");
        check(JudgeImage(U"result_v2_judge.png",palette)==(palette==JudgeColor::kFastBlue?U"result_v2_judge.png":U"result_v2_judge_inversion.png"),U"result uses matching image variant");
    }
    const auto savedPalette=ConfigIni::GetInt(ConfigIni::Key::kJudgeColor,0);
    for(int value:{-1,0,1,2,99}) {
        ConfigIni::SetInt(ConfigIni::Key::kJudgeColor,value);
        check(RuntimeConfig::LoadTimingHudSettingsFromConfigIni().judgeColor==(value==1?JudgeColor::kFastRed:JudgeColor::kFastBlue),U"palette defaults safely and loads through shared configuration");
    }
    ConfigIni::SetInt(ConfigIni::Key::kJudgeColor,savedPalette);
    const std::array<double,4> positions{78,169,299,411};
    for(int i=0;i<4;++i)check(TimingPositionY(static_cast<TimingPosition>(i))==positions[i],U"four fixed HUD anchors match normal-mode reference");
    const auto originalMode=ConfigIni::GetInt(ConfigIni::Key::kShowFastSlow,0);
    const auto originalPosition=ConfigIni::GetInt(ConfigIni::Key::kTimingPosition,0);
    const auto originalDisplay=ConfigIni::GetInt(ConfigIni::Key::kTimingMsDisplay,3);
    const auto originalColor=ConfigIni::GetInt(ConfigIni::Key::kTimingMsColor,2);
    for(int mode:{0,1,2}) {
        ConfigIni::SetInt(ConfigIni::Key::kShowFastSlow,mode);ConfigIni::SetInt(ConfigIni::Key::kTimingPosition,3);
        ConfigIni::SetInt(ConfigIni::Key::kTimingMsDisplay,1);ConfigIni::SetInt(ConfigIni::Key::kTimingMsColor,1);
        ConfigIni::Save();ConfigIni::Load();
        check(ConfigIni::GetInt(ConfigIni::Key::kShowFastSlow,-1)==mode && ConfigIni::GetInt(ConfigIni::Key::kTimingPosition,-1)==3
            && ConfigIni::GetInt(ConfigIni::Key::kTimingMsDisplay,-1)==1 && ConfigIni::GetInt(ConfigIni::Key::kTimingMsColor,-1)==1,U"legacy and NEW+ modes preserve subordinate settings across reload");
    }
    ConfigIni::SetInt(ConfigIni::Key::kShowFastSlow,originalMode);ConfigIni::SetInt(ConfigIni::Key::kTimingPosition,originalPosition);
    for(int cursor=0;cursor<4;++cursor){
        ConfigIni::SetInt(ConfigIni::Key::kTimingMsDisplay,static_cast<int32>(TimingDisplayChoice(cursor)));
        ConfigIni::SetInt(ConfigIni::Key::kTimingMsColor,cursor);ConfigIni::Save();ConfigIni::Load();
        const auto loaded=RuntimeConfig::LoadTimingHudSettingsFromConfigIni();
        check(TimingDisplayCursor(loaded.display)==cursor && static_cast<int32>(loaded.color)==cursor,U"all four persisted display and color choices reload through shared runtime settings");
    }
    ConfigIni::SetInt(ConfigIni::Key::kTimingMsDisplay,originalDisplay);ConfigIni::SetInt(ConfigIni::Key::kTimingMsColor,originalColor);ConfigIni::Save();
    // Drive the real button judgment path, not just the presentation helper.
    kson::ChartData chart;chart.beat.bpm.emplace(0,120);chart.beat.timeSig.emplace(0,kson::TimeSig{});
    const auto cache=kson::CreateTimingCache(chart.beat);
    const auto pulse=kson::SecToPulse(1.0,chart.beat,cache);
    chart.note.bt[0].emplace(pulse,kson::Interval{});
    const double noteSec=kson::PulseToSec(pulse,chart.beat,cache);
    PlayResult lastResult;
    const auto runJudgment=[&](double offset,FastSlowMode mode,bool input,bool automatic,bool fx=false) {
        auto notes=chart;
        if(fx) {
            notes.note.bt[0].clear();notes.note.fx[0].emplace(pulse,kson::Interval{});
            notes.audio.keySound.fx.chipEvent["timing-fixture.wav"][0].emplace(pulse,kson::KeySoundInvokeFX{});
        }
        PlayOption option;option.fastSlowMode=mode;
        if(automatic) {option.btJudgmentPlayMode=JudgmentPlayMode::kAuto;option.fxJudgmentPlayMode=JudgmentPlayMode::kAuto;}
        JudgmentMain judgment{notes,cache,option,none,GameMode::kNormal};GameStatus game;ViewStatus view;
        game.currentTimeSec=game.currentTimeSecForButtonJudgment=noteSec-offset/1000;
        game.currentPulse=game.currentPulseForButtonJudgment=kson::SecToPulse(game.currentTimeSec,notes.beat,cache);
        if(input) KeyConfig::RequestVirtualDown(fx ? kButtonFX_L : kButtonBT_A);
        KeyConfig::Update();judgment.update(notes,game,view);
        const auto sharedResult=judgment.playResult(notes,cache,game.currentTimeSec,IsHardFailedYN::No,false);
        check(view.inputTiming.roundedMeanMs()==sharedResult.timingAverageMs,U"live timing and original FX result read identical real judgment accumulator");
        const auto& live=view.detailedJudgments;const auto& final=sharedResult.detailedStats;
        check(live.fastError==final.fastError && live.fastNear==final.fastNear && live.fastCritical==final.fastCritical
            && live.perfectCritical==final.perfectCritical && live.slowCritical==final.slowCritical
            && live.slowNear==final.slowNear && live.slowError==final.slowError,U"all seven live counters match original result snapshot");
        KeyConfig::ClearInput(fx ? kButtonFX_L : kButtonBT_A);
        lastResult=judgment.playResult(notes,cache,game.currentTimeSec,IsHardFailedYN::No,false);
        if (lastResult.comboStats.critical==1) {
            const auto& lane=fx ? game.fxLaneStatus[0] : game.btLaneStatus[0];
            const bool perfect=lastResult.detailedStats.perfectCritical==1;
            check(lane.keyBeamType==(perfect ? KeyBeamType::kPerfectCritical : KeyBeamType::kCritical),U"actual chip keybeam follows existing perfect-critical count classification");
            bool found=false;
            for(const auto& anim:lane.chipAnim.array())if(anim.type==(perfect ? ChipAnimType::kPerfectCritical : ChipAnimType::kCritical))found=true;
            check(found,U"actual chip effect follows existing perfect-critical count classification");
        }
        return view;
    };
    for (const auto& pair:std::array<std::pair<double,JudgmentResult>,6>{{{12.3,JudgmentResult::kCritical},{43.9,JudgmentResult::kCritical},{44.1,JudgmentResult::kNearFast},{-60.3,JudgmentResult::kNearSlow},{113.9,JudgmentResult::kNearFast},{114.1,JudgmentResult::kError}}}) {
        const auto view=runJudgment(pair.first,FastSlowMode::kNew,true,false);
        check(view.timingFeedback && view.timingFeedback->result==pair.second && std::abs(view.timingFeedback->signedMs-pair.first)<0.0001,U"real BT judgment forwards final result and signed input error");
        const auto original=runJudgment(pair.first,FastSlowMode::kOrigin,true,false);
        check(view.score==original.score && view.gaugePercentage==original.gaugePercentage,U"NEW+ does not change score or gauge versus ORIGIN");
    }
    check(!runJudgment(-150,FastSlowMode::kNew,false,false).timingFeedback,U"real missed note does not generate timing HUD event");
    check(!runJudgment(0,FastSlowMode::kNew,false,true).timingFeedback,U"real autoplay judgment does not generate timing HUD event");
    const auto promoted=runJudgment(60,FastSlowMode::kNew,true,false,true);
    check(promoted.timingFeedback && promoted.timingFeedback->result==JudgmentResult::kCritical,U"FX early key-sound NEAR promotion remains CRITICAL for display filters");
    // Supplemental statistics never alter any existing judgment or score.
    for (double ms : {0.0, 20.999, -20.999, 21.0, -21.0, 21.001, -21.001, 43.9, -43.9}) {
        const auto expected=std::abs(ms)<=21 ? DetailedJudgment::kPerfectCritical
            : ms>0 ? DetailedJudgment::kFastCritical : DetailedJudgment::kSlowCritical;
        check(ClassifyDetailedJudgment(JudgmentResult::kCritical,ms)==expected,U"P-CRITICAL inclusive raw 21ms boundary and signed CRITICAL classification");
    }
    DetailedJudgmentStats stats;
    for(auto result:{JudgmentResult::kCritical,JudgmentResult::kNearFast,JudgmentResult::kNearSlow,JudgmentResult::kError}) {
        stats.add(result,60);stats.add(result,-60);
    }
    stats.add(JudgmentResult::kCritical,0);stats.add(JudgmentResult::kCritical,21);stats.add(JudgmentResult::kCritical,-21);
    stats.add(JudgmentResult::kCritical); // AUTO, long and laser success.
    stats.add(JudgmentResult::kError); // Miss / hold / laser failure.
    check(stats.fastError==1 && stats.slowError==2 && stats.fastNear==2 && stats.slowNear==2
        && stats.fastCritical==1 && stats.slowCritical==1 && stats.perfectCritical==4,U"all seven counters remain independent, unmeasured success is P-CRITICAL, miss is SLOW ERROR");
    for(double ms:{12.3,21.001,43.9,44.1,114.1,-12.3,-21.001,-43.9,-60.3}) {
        runJudgment(ms,FastSlowMode::kNew,true,false);
        const auto& d=lastResult.detailedStats;
        check(d.fastError+d.fastNear+d.fastCritical+d.perfectCritical+d.slowCritical+d.slowNear+d.slowError==1,U"real BT produces exactly one detail count");
        check(d.fastCritical+d.perfectCritical+d.slowCritical==lastResult.comboStats.critical
            && d.fastNear+d.slowNear==lastResult.comboStats.totalNear()
            && d.fastError+d.slowError==lastResult.comboStats.error,U"detail subtotals match existing totals in real judgment pipeline");
        check((d.perfectCritical==1)==(std::abs(ms)<=21),U"real input P-CRITICAL uses unrounded error");
    }
    runJudgment(60,FastSlowMode::kNew,true,false,true);
    check(lastResult.comboStats.critical==1 && lastResult.detailedStats.fastCritical==1,U"FX promoted CRITICAL retains score and uses actual direction");
    runJudgment(0,FastSlowMode::kNew,false,true);
    check(lastResult.comboStats.critical==1 && lastResult.detailedStats.perfectCritical==1,U"AUTO CRITICAL contributes P-CRITICAL");
    runJudgment(-150,FastSlowMode::kNew,false,false);
    check(lastResult.comboStats.error==1 && lastResult.detailedStats.slowError==1,U"miss contributes SLOW ERROR without fabricated ms");
    // Real long and laser judgment callbacks (AUTO and missed input).
    for(bool automatic:{false,true}) {
        auto notes=chart;notes.note.bt[0].clear();
        notes.note.bt[0].emplace(pulse,kson::Interval{96});
        kson::LaserSection laser;laser.v.emplace(0,kson::GraphPoint{0.0});laser.v.emplace(96,kson::GraphPoint{1.0});
        notes.note.laser[0].emplace(pulse,laser);
        PlayOption option;option.isAutoPlay=IsAutoPlayYN{automatic};
        JudgmentMain judgment{notes,cache,option,none,GameMode::kNormal};GameStatus game;ViewStatus view;
        for(int frame=0;frame<=400;++frame) {
            game.currentTimeSec=game.currentTimeSecForButtonJudgment=game.currentTimeSecForLaserJudgment=frame/100.0;
            game.currentPulse=game.currentPulseForButtonJudgment=game.currentPulseForLaserJudgment=kson::SecToPulse(game.currentTimeSec,notes.beat,cache);
            judgment.update(notes,game,view);
        }
        const auto result=judgment.playResult(notes,cache,4.0,IsHardFailedYN::No,false);
        check(result.longOrLaserLineStats.totalJudgedCombo()>0,U"real long/laser fixture exercises sustained judgments");
        check(result.detailedStats.perfectCritical==result.comboStats.critical
            && result.detailedStats.slowError==result.comboStats.error,U"real long/laser success is P-CRITICAL and failure is SLOW ERROR");
        check(result.detailedStats.fastCritical==0 && result.detailedStats.slowCritical==0,U"long/laser has no fabricated directional timing");
    }
    // Count every simultaneous judgment even though the HUD picks one representative.
    {
        auto notes=chart;notes.note.bt[1].emplace(pulse,kson::Interval{});
        PlayOption option;JudgmentMain judgment{notes,cache,option,none,GameMode::kNormal};GameStatus game;ViewStatus view;
        game.currentTimeSec=game.currentTimeSecForButtonJudgment=noteSec-0.03;
        game.currentPulse=game.currentPulseForButtonJudgment=kson::SecToPulse(game.currentTimeSec,notes.beat,cache);
        KeyConfig::RequestVirtualDown(kButtonBT_A);KeyConfig::RequestVirtualDown(kButtonBT_B);KeyConfig::Update();
        judgment.update(notes,game,view);KeyConfig::ClearInput(kButtonBT_A);KeyConfig::ClearInput(kButtonBT_B);
        const auto result=judgment.playResult(notes,cache,game.currentTimeSec,IsHardFailedYN::No,false);
        check(result.comboStats.critical==2 && result.detailedStats.fastCritical==2,U"simultaneous notes count individually, independent of HUD representative");
    }
    report.writeln(U"TOTAL {} checks passed"_fmt(checks));
}
}
