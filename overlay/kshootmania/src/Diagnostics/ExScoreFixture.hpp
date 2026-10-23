#pragma once
#include "MusicGame/Judgment/ExScore.hpp"
#include "MusicGame/Judgment/JudgmentMain.hpp"
#include "HighScore/KscIO.hpp"
#include "Graphics/ExScorePanel.hpp"
#include "Graphics/ResultBestScorePanel.hpp"
#include "Ini/ConfigIni.hpp"
namespace ExScoreFixture {
inline void Run(){
    using namespace MusicGame;using namespace MusicGame::Judgment;
    if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture")))throw Error{U"Isolated EX fixture required"};
    TextWriter report{FsUtils::GetResourcePath(U"ex-score-test.txt")};int checks=0;
    const auto check=[&](bool ok,StringView text){if(!ok)throw Error{String{text}};++checks;report.writeln(U"PASS "+String{text});};
    for(double ms:{-21.0,0.0,21.0})check(ExChipPoints(JudgmentResult::kCritical,ms)==5,U"inclusive P-CRITICAL chip window");
    for(double ms:{-21.001,21.001})check(ExChipPoints(JudgmentResult::kCritical,ms)==4,U"ordinary CRITICAL chip points");
    check(ExChipPoints(JudgmentResult::kNearFast,75)==2 && ExChipPoints(JudgmentResult::kNearSlow,-75)==2,U"NEAR directions have two points");
    check(ExChipPoints(JudgmentResult::kError,150)==0 && ExTickPoints(JudgmentResult::kError)==0,U"ERROR never adds EX");
    check(ExTickPoints(JudgmentResult::kCritical)==2,U"long line and slam success is two points");
    for(auto score:{9800LL,9801LL,9950LL,10000LL})check(ExPanelKind(score,10000)==(score==10000?3:score>9800?2:1),U"exact 98 percent and MAX boundary");
    check(ExPercentHundredths(97999,100000)==9799,U"percentage truncates rather than rounds");
    check(ExPanelKind(0,10000)==1 && ExPanelKind(0,0)==0 && ExPanelKind(10001,10000)==0,U"zero score and invalid maximum distinguished");
    for(double bpm:{120.0,300.0,600.0})for(double step:{0.001,0.035,0.75}){
        kson::ChartData chart;chart.beat.bpm.emplace(0,bpm);chart.beat.timeSig.emplace(0,kson::TimeSig{});
        for(int i=0;i<4;++i){chart.note.bt[i][kson::kResolution4]=kson::Interval{};chart.note.bt[i][kson::kResolution4*2]=kson::Interval{kson::kResolution4};}
        for(int i=0;i<2;++i){chart.note.fx[i][kson::kResolution4]=kson::Interval{};chart.note.fx[i][kson::kResolution4*2]=kson::Interval{kson::kResolution4};}
        kson::LaserSection section;section.v[0]=0.2;section.v[kson::kResolution4/2]=kson::GraphValue{0.2,0.8};section.v[kson::kResolution4]=0.3;chart.note.laser[0][kson::kResolution4]=section;
        const auto timing=kson::CreateTimingCache(chart.beat);const auto maximum=ChartExMaximum(chart);
        PlayOption option;option.isAutoPlay=IsAutoPlayYN::Yes;option.diagnosticSession=true;
        JudgmentMain judgment{chart,timing,option,none,GameMode::kNormal};GameStatus game;ViewStatus view;
        for(double time=0;time<6+step;time+=step){
            game.currentTimeSec=game.currentTimeSecForButtonJudgment=game.currentTimeSecForLaserJudgment=time;
            game.currentPulse=game.currentPulseForButtonJudgment=game.currentPulseForLaserJudgment=kson::SecToPulse(time,chart.beat,timing);
            judgment.update(chart,game,view);
        }
        const auto result=judgment.playResult(chart,timing,6,IsHardFailedYN::No,false);
        check(maximum>30 && result.exMaximum==maximum && result.exScore==maximum,U"all-success actual BT FX long line slam judgments reach precomputed maximum at every BPM and FPS");
        check(result.score==10000000,U"EX leaves original perfect score unchanged");
        const auto before=view.exScore;judgment.update(chart,game,view);check(before==view.exScore,U"repeated frame cannot duplicate EX");
        option.isAutoPlay=IsAutoPlayYN::No;option.btJudgmentPlayMode=option.fxJudgmentPlayMode=option.laserJudgmentPlayMode=JudgmentPlayMode::kOff;
        JudgmentMain disabled{chart,timing,option,none,GameMode::kNormal};ViewStatus offView;disabled.update(chart,game,offView);
        check(offView.exScore==0 && offView.exMaximum==maximum,U"disabled lanes do not reduce full-chart maximum");
    }
    const String savedPlayer{ConfigIni::GetString(ConfigIni::Key::kCurrentPlayer)};ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,U"EX Fixture");
    const FilePath song=FileSystem::PathAppend(FsUtils::SongsDirectoryPath(),U"EX/test.ksh");
    FileSystem::CreateDirectories(FileSystem::ParentPath(song));{TextWriter w{song};w.writeln(U"title=EX\n--\n0000|00|--\n--");}
    const auto original=FileSystem::PathAppend(FsUtils::ScoreDirectoryPath(),U"EX Fixture/EX/test.ksc");
    FileSystem::CreateDirectories(FileSystem::ParentPath(original));
    {TextWriter w{original,TextEncoding::UTF8_NO_BOM};w.writeln(U"normal,normal,x09,on,on,on=100,0,1,0,1,1,0,0,0");}
    check(!KscIO::ReadHighScoreInfo(song,KscKey{}).exScore(GaugeType::kNormalGauge),U"legacy records have no fabricated EX");
    PlayResult result;result.score=9000000;result.exScore=90;result.exMaximum=100;result.totalCombo=10;result.comboStats.critical=10;
    check(KscIO::WriteHighScoreInfo(song,result,KscKey{}),U"first additional record writes successfully");
    result.score=8800000;result.exScore=95;check(KscIO::WriteHighScoreInfo(song,result,KscKey{}),U"EX-only new record writes successfully");
    auto info=KscIO::ReadHighScoreInfo(song,KscKey{});
    check(info.score(GaugeType::kNormalGauge)==9000000 && info.exScore(GaugeType::kNormalGauge)==Optional<int64>{95},U"independent EX best does not replace ordinary best");
    check(info.playCount(GaugeType::kNormalGauge)==2,U"original play count increments once per play");
    result.score=9500000;result.exScore=92;check(KscIO::WriteHighScoreInfo(song,result,KscKey{}),U"ordinary-only new record writes successfully");
    check(KscIO::ReadHighScoreInfo(song,KscKey{}).exScore(GaugeType::kNormalGauge)==Optional<int64>{95},U"ordinary new record preserves EX best");
    auto hard=KscKey{}.withGaugeType(GaugeType::kHardGauge);result.playOption.gaugeType=GaugeType::kHardGauge;result.exScore=97;check(KscIO::WriteHighScoreInfo(song,result,hard),U"HARD record writes");
    check(KscIO::ReadHighScoreInfo(song,KscKey{}).exScore(GaugeType::kNormalGauge)==Optional<int64>{97},U"NORMAL and HARD share EX best");
    auto easy=KscKey{}.withGaugeType(GaugeType::kEasyGauge);result.playOption.gaugeType=GaugeType::kEasyGauge;result.exScore=0;check(KscIO::WriteHighScoreInfo(song,result,easy),U"valid zero EX writes");
    check(KscIO::ReadHighScoreInfo(song,easy).exScore(GaugeType::kEasyGauge)==Optional<int64>{0},U"EASY keeps separate valid zero record");
    HashTable<String,HighScoreInfo> all;KscIO::ReadAllHighScoreInfo(song,&all);
    check(!all[U"normal,x09,on,on,on"].exScore(GaugeType::kNormalGauge),U"copied legacy option row remains unrecorded");
    const String oldFile=TextReader{original}.readAll();result.playOption.isAutoPlay=IsAutoPlayYN::Yes;
    check(!KscIO::WriteHighScoreInfo(song,result,easy) && TextReader{original}.readAll()==oldFile,U"ordinary AUTO cannot modify either record");
    const auto additional=FsUtils::EliminateExtension(original)+U"_additional.ksc";
    TextReader a{original},b{additional};String line,extra;
    while(a.readLine(line)){if(line.isEmpty())continue;check(b.readLine(extra) && extra.substr(0,extra.lastIndexOf(U','))==line,U"additional first nine fields mirror every original option row");}
    ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,U"EX Other");check(!KscIO::ReadHighScoreInfo(song,KscKey{}).exScore(GaugeType::kNormalGauge),U"player records remain isolated");
    ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,U"EX Fixture");
    const auto blockedSong=FileSystem::PathAppend(FsUtils::SongsDirectoryPath(),U"EX/blocked.ksh");
    const auto blocked=FileSystem::PathAppend(FsUtils::ScoreDirectoryPath(),U"EX Fixture/EX/blocked_additional.ksc");
    FileSystem::CreateDirectories(blocked);result.playOption.isAutoPlay=IsAutoPlayYN::No;
    check(KscIO::WriteHighScoreInfo(blockedSong,result,easy),U"additional replace failure does not undo original score save");
    check(KscIO::ReadHighScoreInfo(blockedSong,easy).score(GaugeType::kEasyGauge)==9500000 && FileSystem::IsDirectory(blocked),U"original remains valid when additional path is blocked");
    FileSystem::Remove(blocked);
    ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,savedPlayer);
    for(auto pair:{std::pair<int64,int64>{6630,6800},{6698,6800},{6800,6800},{0,6800},{123456,123456},{8,8}}){
        auto image=ExScorePanel::Compose(pair.first,pair.second);check(image.size()==Size{228,46},U"composed panel has three-pixel glow margin");
        image.save(FsUtils::GetResourcePath(U"ex-panel-{}.png"_fmt(pair.first)));
    }
    {
        const auto saved=SkinPreset::Current();const auto root=FsUtils::GetResourcePath(U"imgs");
        for(const auto folder:{U"EX Complete",U"EX Partial"})FileSystem::CreateDirectories(FileSystem::PathAppend(root,folder));
        FileSystem::Copy(FileSystem::PathAppend(root,U"default/num0.png"),FileSystem::PathAppend(root,U"EX Complete/num0.png"));
        FileSystem::Copy(FileSystem::PathAppend(root,U"default/num0.png"),FileSystem::PathAppend(root,U"EX Partial/num0.png"));
        for(int i=1;i<=3;++i){
            const String name=U"msel_exscore_{}.png"_fmt(i);
            FileSystem::Copy(FileSystem::PathAppend(root,U"default/"+name),FileSystem::PathAppend(root,U"EX Complete/"+name));
            SkinPreset::Activate({U"EX Complete",SkinPreset::MissingMode::Default});
            check(SkinPreset::Resolve(name).includes(U"EX Complete"),U"complete user skin owns EX resource");
            SkinPreset::Activate({U"EX Partial",SkinPreset::MissingMode::Default});
            check(SkinPreset::Resolve(name).includes(U"default"),U"partial EX skin uses Default resource");
            SkinPreset::Activate({U"EX Partial",SkinPreset::MissingMode::Error});
            check(Image{SkinPreset::Resolve(name)}[0][0]==Palette::Yellow,U"partial EX skin uses Yellow missing resource");
        }
        SkinPreset::Activate(saved);
    }

    {
        PlayResult current;current.score=215866;current.exScore=6630;
        HighScoreInfo previous;previous.normalGauge.score=159201;previous.normalGauge.exScore=6600;
        const auto captured=ResultBestScoreComparison::Capture(previous,current);
        check(captured.visible && captured.bestScore==159201 && captured.bestEx==6600 && captured.scoreDelta==56665 && captured.exDelta==30,U"checker snapshots independent bests and positive signed differences");
        previous.normalGauge.score=10000000;previous.normalGauge.exScore=6800;
        check(captured.bestScore==159201 && captured.bestEx==6600,U"checker snapshot survives later record update");
        const auto negative=ResultBestScoreComparison::Capture(previous,current);
        check(negative.scoreDelta==-9784134 && negative.exDelta==-170,U"checker negative differences do not underflow");
        current.score=10000000;current.exScore=6800;
        const auto tied=ResultBestScoreComparison::Capture(previous,current);
        check(tied.scoreDelta==0 && tied.exDelta==0,U"checker tie is zero in both records");
        previous=HighScoreInfo{};const auto first=ResultBestScoreComparison::Capture(previous,current);
        check(first.bestScore==0 && first.bestEx==0 && first.scoreDelta==10000000 && first.exDelta==6800,U"checker first play compares against zero");
        previous.normalGauge.score=9000000;const auto legacy=ResultBestScoreComparison::Capture(previous,current);
        check(legacy.bestScore==9000000 && legacy.bestEx==0 && legacy.exDelta==6800,U"checker legacy ordinary record does not invent prior EX");
        for(int mode=0;mode<4;++mode){
            PlayResult excluded=current;
            if(mode==0)excluded.playOption.isAutoPlay=IsAutoPlayYN::Yes;
            if(mode==1)excluded.playOption.skinPreview=true;
            if(mode==2)excluded.playOption.diagnosticSession=true;
            if(mode==3)excluded.playOption.testPlayOption.emplace();
            check(!ResultBestScoreComparison::Capture(previous,excluded).visible,U"checker hidden for every non-saving play mode");
        }
        {
            const auto savedSkin=SkinPreset::Current();const auto root=FsUtils::GetResourcePath(U"imgs");
            FileSystem::Copy(FileSystem::PathAppend(root,U"default/result_bestscore_checker.png"),FileSystem::PathAppend(root,U"EX Complete/result_bestscore_checker.png"));
            SkinPreset::Activate({U"EX Complete",SkinPreset::MissingMode::Default});
            check(SkinPreset::Resolve(U"result_bestscore_checker.png").includes(U"EX Complete"),U"checker prefers selected skin background");
            SkinPreset::Activate({U"EX Partial",SkinPreset::MissingMode::Default});
            check(SkinPreset::Resolve(U"result_bestscore_checker.png").includes(U"default"),U"checker partial skin uses Default background");
            SkinPreset::Activate({U"EX Partial",SkinPreset::MissingMode::Error});
            check(Image{SkinPreset::Resolve(U"result_bestscore_checker.png")}[0][0]==Palette::Yellow,U"checker partial skin uses Yellow missing marker");
            SkinPreset::Activate(savedSkin);
        }
        int index=0;
        for(const auto data:{captured,negative,tied,first,legacy,ResultBestScoreComparison{true,0,0,0,0},ResultBestScoreComparison{true,10000000,19999LL,0,-19999LL}}){
            const auto image=ResultBestScorePanel::Compose(data);
            check(image.size()==Size{248,214},U"checker composed at exact resource size plus two-pixel glow margin");
            image.save(FsUtils::GetResourcePath(U"best-checker-{}.png"_fmt(index++)));
        }
        a.close();b.close();
        // Capture from the actual independent high-score file, then update it.
        ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,U"EX Fixture");
        PlayResult next;next.score=9600000;next.exScore=98;
        const auto actualPrevious=KscIO::ReadHighScoreInfo(song,KscKey{});
        const auto snapshot=ResultBestScoreComparison::Capture(actualPrevious,next);
        check(snapshot.bestScore==9500000 && snapshot.bestEx==97,U"checker uses actual NORMAL HARD independent record lookup");
        check(KscIO::WriteHighScoreInfo(song,next,KscKey{}),U"checker record update fixture writes once");
        check(snapshot.bestScore==9500000 && snapshot.bestEx==97 && snapshot.scoreDelta==100000 && snapshot.exDelta==1,U"checker prior values remain unchanged after persistent save");
        ConfigIni::SetString(ConfigIni::Key::kCurrentPlayer,savedPlayer);
    }
    {
        const auto saved=SkinPreset::Current();const auto root=FsUtils::GetResourcePath(U"imgs");
        FileSystem::Copy(FileSystem::PathAppend(root,U"default/score_header_ex.png"),FileSystem::PathAppend(root,U"EX Complete/score_header_ex.png"));
        SkinPreset::Activate({U"EX Complete",SkinPreset::MissingMode::Default});
        check(SkinPreset::Resolve(U"score_header_ex.png").includes(U"EX Complete"),U"ingame EX banner prefers selected skin");
        SkinPreset::Activate({U"EX Partial",SkinPreset::MissingMode::Default});
        check(SkinPreset::Resolve(U"score_header_ex.png").includes(U"default"),U"ingame EX banner follows Default missing policy");
        SkinPreset::Activate({U"EX Partial",SkinPreset::MissingMode::Error});
        check(Image{SkinPreset::Resolve(U"score_header_ex.png")}[0][0]==Palette::Yellow,U"ingame EX banner follows Yellow missing policy");
        SkinPreset::Activate(saved);
    }
    for(const int64 value:{7LL,30LL,102LL,1234LL,19999LL}){
        for(const int direction:{-1,1})
            ResultBestScorePanel::Compose({true,10000000,19999,direction*12345,direction*value})
                .save(FsUtils::GetResourcePath(U"best-sign-{}-{}.png"_fmt(value,direction)));
    }
    report.writeln(U"TOTAL {} EX checks passed"_fmt(checks));
}
}
