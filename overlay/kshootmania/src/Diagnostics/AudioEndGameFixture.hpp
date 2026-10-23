#pragma once
#include "MusicGame/GameMain.hpp"
#include "Common/FsUtils.hpp"
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif
namespace AudioEndGameFixture {
inline void Run() {
#ifdef _WIN32
    using namespace MusicGame;
    if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Requires isolated audio fixture"};
    const auto render=reinterpret_cast<DWORD(WINAPI*)(DWORD,float*)>(::GetProcAddress(::GetModuleHandleW(L"bassasio.dll"),"FakeAsioRender"));
    if(!render || !ksmaudio::IsASIOActive()) throw Error{U"Requires mock ASIO"};
    TextWriter report{FsUtils::GetResourcePath(U"audio-end-game.txt")};
    const auto check=[&](bool ok,StringView text){if(!ok)throw Error{String{text}};report.writeln(U"PASS "+String{text});};
    for(bool automatic:{false,true}) {
        GameCreateInfo info;
        info.chartFilePath=U"eof-song/magicalkittens_ex.ksh";
        info.playOption.diagnosticSession=true;
        info.playOption.isAutoPlay=IsAutoPlayYN{automatic};
        info.playOption.movieEnabled=false;
        info.playOption.testPlayOption=TestPlayOption{.startMeasure=81};
        GameMain game{info};game.start();
        std::vector<float> pcm(2048);bool ready=false;double previous=0;bool monotonic=true;
        for(int i=0;i<900 && !ready;++i) {
            if(!System::Update())throw Error{U"Fixture window closed"};
            check(render(1024,pcm.data())==8192,U"game output callback completes");
            ready=static_cast<bool>(game.update());
            const double now=game.previewTime();if(i)monotonic&=now>=previous-0.001;previous=now;
        }
        check(monotonic,U"real chart game time is monotonic through EOF");
        check(ready,U"real chart requests result transition after final judgments and 2.4 seconds");
        report.writeln(U"GAME automatic={} finalTime={:.6f}"_fmt(automatic,game.previewTime()));
        check(!info.playOption.shouldSaveScore(),U"diagnostic chart cannot save score");
        game.terminate();
    }
    if(System::GetCommandLineArgs().contains(U"--aps-game-fixture")) {
        for(bool automatic:{false,true}) {
            GameCreateInfo info;info.chartFilePath=U"eof-song/magicalkittens_ex.ksh";
            info.playOption.gaugeType=GaugeType::kHardGauge;info.playOption.apsEnabled=true;
            info.playOption.isAutoPlay=IsAutoPlayYN{automatic};info.playOption.movieEnabled=false;
            GameMain game{info};game.start();std::vector<float> pcm(65536);
            bool ready=false,sawHard=false,sawNormal=false,continued=false;double switchTime=0;
            for(int i=0;i<2600 && !ready;++i){
                if(!System::Update())throw Error{U"APS fixture window closed"};
                check(render(32768,pcm.data())==32768*8,U"APS ASIO callback remains active");
                ready=static_cast<bool>(game.update());const auto& view=game.previewView();
                sawHard |= view.effectiveGaugeType==GaugeType::kHardGauge && view.apsProtected;
                if(view.effectiveGaugeType==GaugeType::kNormalGauge){if(!sawNormal)switchTime=game.previewTime();sawNormal=true;continued |= game.previewTime()>switchTime+10;}
            }
            const auto result=game.playResult();
            check(ready && sawHard,U"APS full real chart reaches result transition");
            check(automatic ? !sawNormal && result.playOption.gaugeType==GaugeType::kHardGauge : sawNormal && continued && result.playOption.gaugeType==GaugeType::kNormalGauge,U"real GameMain either survives HARD or continues over ten seconds after NORMAL fallback");
            check(!result.isHardFailed,U"APS fallback never reports HARD failure");
            check(result.comboStats.critical+result.comboStats.nearFast+result.comboStats.nearSlow+result.comboStats.error==result.totalCombo,U"APS full chart judges all notes exactly once");
            if(automatic)check(result.exScore==result.exMaximum,U"APS survivor preserves perfect EX");
            report.writeln(U"APS GAME automatic={} finalTime={} switched={}"_fmt(automatic,game.previewTime(),sawNormal));
            game.terminate();
        }
    }
    report.writeln(U"TOTAL game EOF cases passed");
#endif
}
}

