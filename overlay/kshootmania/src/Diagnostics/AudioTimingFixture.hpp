#pragma once
#include "MusicGame/Audio/BGM.hpp"
#include "MusicGame/Audio/AudioTailClock.hpp"
#include "MusicGame/Audio/AudioDefines.hpp"
#include "MusicGame/Audio/AssistTick.hpp"
#include "MusicGame/Audio/FXChipSE.hpp"
#include "MusicGame/Audio/LaserSlamSE.hpp"
#include "Common/FsUtils.hpp"
#include "Input/KeyConfig.hpp"
#include "Ini/ConfigIni.hpp"
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif
namespace AudioTimingFixture {
inline void Run() {
#ifdef _WIN32
    using namespace MusicGame;
    using namespace MusicGame::Audio;
    if (!FileSystem::Exists(FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"audio-test-fixture")))
        throw Error{U"Timing tests require fixture directory"};
    const auto render=reinterpret_cast<DWORD (WINAPI*)(DWORD,float*)>(::GetProcAddress(::GetModuleHandleW(L"bassasio.dll"),"FakeAsioRender"));
    if(!render || !ksmaudio::IsASIOActive()) throw Error{U"Timing tests require mock ASIO"};
    TextWriter report{FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"audio-timing-test.txt")};
    size_t checks=0;
    const auto check=[&](bool condition,StringView name) { if(!condition) throw Error{String{name}}; ++checks;report.writeln(U"PASS " + String{name}); };
    std::vector<float> buffer(256);
    const auto pump=[&] { check(render(128,buffer.data())==1024,U"complete output block"); };
    const auto count=[] { return ksmaudio::GetAudioLatencySnapshot().scheduledEvents; };
    const auto context=[&](std::uint64_t generation=100) {
        ksmaudio::AudioClockSnapshot clock;
        clock.valid=clock.playing=true;clock.generation=generation;
        clock.outputFrame=ksmaudio::GetAudioLatencySnapshot().outputFrame;
        BeginScheduledAudioFrame(clock,0.016);
    };
    ksmaudio::SetMasterVolume(1);
    kson::ChartData chart;
    chart.beat.bpm.emplace(0,120);chart.beat.timeSig.emplace(0,kson::TimeSig{});
    const auto timing=kson::CreateTimingCache(chart.beat);
    {
        BGM bgm{U"timing-fixture.wav",0,SecondsF{0.025},LegacyAudioFPMode::kNone,chart,U"."};
        bgm.seekPosSec(SecondsF{0.5});bgm.play();bgm.update();
        for(int i=0;i<10;++i)pump();bgm.update();
        const auto clock=bgm.clockSnapshot();const auto time=bgm.posSec().count();
        check(clock.valid && std::abs(clock.audiblePositionSec-time)<1e-9,U"game time and scheduled clock share one snapshot");
        pump();check(bgm.clockSnapshot().outputFrame==clock.outputFrame && bgm.posSec().count()==time,U"callback between game updates cannot change scheduling anchor");
        check(bgm.renderPosSec().count()>clock.renderPositionSec,U"FX uses current mixer render position");
        BeginScheduledAudioFrame(clock,0.016);
        ksmaudio::Sample sound{"timing-fixture.wav"};
        const double userDelay=0.020;
        check(PlayScheduledSound(sound,time+0.050,time-userDelay),U"chart sound accepts manual audio offset");
        const auto rate=ksmaudio::GetAudioOutputStatus().sampleRate;
        const auto expected=clock.outputFrame+static_cast<std::uint64_t>(std::llround((0.070-ksmaudio::GetAudioOutputStatus().latencySec)*rate));
        std::uint64_t onset=0;
        for(int i=0;i<40 && !onset;++i) {
            const auto frame=ksmaudio::GetAudioLatencySnapshot().outputFrame;pump();
            for(size_t n=0;n<128;++n)if(std::abs(buffer[n*2])>0.1f){onset=frame+n;break;}
        }
        check(onset && std::abs(static_cast<double>(onset)-expected)<=1,U"scheduled chart onset includes manual offset once within one sample");
        bgm.seekPosSec(SecondsF{0.8});bgm.update();
        check(std::abs(bgm.posSec().count()-0.8)<0.0001,U"ASIO seek applies content/global offset once while playing");
        bgm.seekPosSec(SecondsF{3.0});bgm.update();
        check(bgm.posSec().count()>=3.0 && !bgm.clockSnapshot().valid,U"seek past audio end uses transport time without restarting music");
        bgm.seekPosSec(SecondsF{-0.1});bgm.update();
        check(bgm.posSec().count()<0 && !bgm.clockSnapshot().valid,U"seek to preroll clears previous source clock");
        bgm.seekPosSec(SecondsF{0.8});bgm.update();
        check(std::abs(bgm.posSec().count()-0.8)<0.0001 && bgm.clockSnapshot().valid,U"seek from preroll restarts grouped audio at correct offset");
        bgm.pause();check(!bgm.clockSnapshot().valid,U"paused game clock cannot schedule future chart sounds");
    }
    {
        BGM tail{U"timing-fixture.wav",0,SecondsF{0},LegacyAudioFPMode::kNone,chart,U"."};
        tail.seekPosSec(SecondsF{1.98});tail.play();tail.update();
        for(int i=0;i<20;++i){render(128,buffer.data());tail.update();}
        const auto before=tail.clockSnapshot();
        for(int i=0;i<10;++i){render(128,buffer.data());tail.update();}
        const auto after=tail.clockSnapshot();
        const auto latency=ksmaudio::GetAudioLatencySnapshot();
        check(tail.posSec().count()>2.0,U"game timeline continues after the audio file ends");
        check(std::abs((after.audiblePositionSec-before.audiblePositionSec)-1280.0/latency.output.sampleRate)<0.0001,U"outro advances on mixer frames rather than wall-clock blend");
        check(std::abs(after.renderPositionSec-after.audiblePositionSec-latency.output.latencySec)<0.0001,U"outro scheduling retains single latency compensation");
    }
    {
        ksmaudio::AudioClockSnapshot clock;clock.valid=true;clock.playing=false;
        constexpr double end=119.289297052;
        for(double rate:{44100.0,48000.0,96000.0}) {
            clock.playing=false;
            for(double shortfall:{0.0005,0.013062052,0.25}) {
                clock.renderPositionSec=end-shortfall;
                check(ShouldStartTailClock(clock,end,rate),U"decoder EOF starts tail despite rounded, 13ms or truncated end position");
                clock.playing=true;
                check(!ShouldStartTailClock(clock,end,rate),U"active or stalled stream short of EOF retains original audio clock");
                clock.playing=false;
            }
            clock.renderPositionSec=end;clock.playing=true;
            check(ShouldStartTailClock(clock,end,rate),U"exact metadata end still starts tail before EOF status update");
            clock.valid=false;check(!ShouldStartTailClock(clock,end,rate),U"invalid source clock cannot arm EOF");clock.valid=true;
        }
        check(!ShouldStartTailClock(clock,end,0),U"invalid device rate cannot divide tail clock by zero");
    }
    // Real OGG from the reported session: the mixer stops before advertised EOF.
    {
        ksmaudio::Stream source{"eof-regression.ogg",0};
        source.seekPosSec(ksmaudio::SecondsF{source.duration().count()-0.1});source.play();
        for(int i=0;i<160;++i)render(128,buffer.data());
        const auto clock=source.clockSnapshot();
        check(clock.valid && !clock.playing,U"reported OGG reaches decoder EOF");
        report.writeln(U"REGRESSION raw EOF gap ms={:.6f}"_fmt((source.duration().count()-clock.renderPositionSec)*1000));
    }
    for(const auto file:{U"timing-fixture.wav",U"eof-regression.ogg"})
    for(double speed:{0.75,1.0,1.5}) for(double offset:{-0.18,0.0,0.18}) {
        report.writeln(U"CASE {} speed={} offset={}"_fmt(file,speed,offset));
        BGM tail{file,0,SecondsF{offset},LegacyAudioFPMode::kNone,chart,U".",speed};
        const double end=tail.duration().count()-offset;
        tail.seekPosSec(SecondsF{end-0.08});tail.play();tail.update();
        double previous=tail.posSec().count();bool monotonic=true;
        for(int i=0;i<120;++i){render(128,buffer.data());tail.update();
            const double now=tail.posSec().count();monotonic&=now>=previous-0.0001;previous=now;}
        check(monotonic && tail.posSec().count()>end+0.2,U"WAV/OGG EOF advances monotonically at every speed and offset");
        auto before=tail.clockSnapshot();
        for(int i=0;i<900;++i){render(128,buffer.data());tail.update();}
        auto after=tail.clockSnapshot();auto latency=ksmaudio::GetAudioLatencySnapshot();
        check(after.audiblePositionSec-before.audiblePositionSec>=2.4-0.000001,U"EOF clock reaches normal result transition delay");
        check(std::abs((after.audiblePositionSec-before.audiblePositionSec)-115200.0/latency.output.sampleRate)<0.0001,U"EOF uses output time at altered speed without double speed scaling");
        check(std::abs(tail.renderPosSec().count()-after.renderPositionSec)<0.0001,U"FX render clock follows the EOF scheduling timeline");
        tail.update();check(tail.posSec().count()==after.audiblePositionSec,U"no callback means no invented EOF time");
        tail.pause();const auto paused=tail.posSec().count();
        for(int i=0;i<80;++i){render(128,buffer.data());tail.update();}
        check(tail.posSec().count()==paused && !tail.clockSnapshot().valid,U"pause freezes EOF and disables scheduling");
        tail.play();tail.update();
        check(std::abs(tail.posSec().count()-paused)<0.0001,U"resume after EOF excludes time spent paused");
        for(int i=0;i<20;++i){render(128,buffer.data());tail.update();}
        check(std::abs(tail.posSec().count()-paused-2560.0/latency.output.sampleRate)<0.0001,U"resumed EOF advances on new frames only");
        tail.seekPosSec(SecondsF{end-0.06});tail.update();
        for(int i=0;i<100;++i){render(128,buffer.data());tail.update();}
        check(tail.posSec().count()>end,U"backward seek clears EOF and reaches it again");
        tail.restartAtChartTime(SecondsF{end-0.07});tail.play();tail.update();
        for(int i=0;i<100;++i){render(128,buffer.data());tail.update();}
        check(tail.posSec().count()>end,U"restart clears EOF epoch before repeated completion");
        tail.seekPosSec(SecondsF{end+1});tail.update();
        check(tail.posSec().count()>=end+1-0.0001 && !tail.clockSnapshot().valid,U"forward seek past EOF does not restart ended music");
        tail.pause();tail.seekPosSec(SecondsF{end-0.05});tail.update();
        for(int i=0;i<30;++i){render(128,buffer.data());tail.update();}
        check(std::abs(tail.posSec().count()-(end-0.05))<0.0001,U"paused seek before EOF cannot run the tail clock");
        tail.play();tail.update();
        for(int i=0;i<100;++i){render(128,buffer.data());tail.update();}
        check(tail.posSec().count()>end,U"resume after paused seek completes normally");
    }
    {
        BGM tail{U"timing-fixture.wav",0,SecondsF{0},LegacyAudioFPMode::kNone,chart,U"."};
        tail.seekPosSec(SecondsF{1.95});tail.play();tail.update();
        for(int i=0;i<100;++i){render(128,buffer.data());tail.update();}
        const double before=tail.posSec().count();
        const auto driver=ksmaudio::GetAudioOutputStatus().driver;
        ksmaudio::SetMute(true);
        const auto native=ksmaudio::ApplyOutputChange(s3d::Platform::Windows::Window::GetHWND(),{ksmaudio::AudioBackend::DirectSound,{}});
        check(native.success,U"EOF output replacement reaches native backend");
        const auto asio=ksmaudio::ApplyOutputChange(s3d::Platform::Windows::Window::GetHWND(),{ksmaudio::AudioBackend::ASIO,driver});
        check(asio.success,U"EOF output replacement returns to ASIO");
        tail.update();check(std::abs(tail.posSec().count()-before)<0.0001,U"output generation reset reanchors EOF without jumps or unsigned underflow");
        for(int i=0;i<100;++i){render(128,buffer.data());tail.update();}
        check(tail.posSec().count()>before+0.2,U"EOF continues after output replacement");
        ksmaudio::SetMute(false);
    }
    {
        static_assert(static_cast<int>(AssistTickMode::kOff) == 0);
        static_assert(static_cast<int>(AssistTickMode::kAuto) == 1);
        static_assert(static_cast<int>(AssistTickMode::kInput) == 2);
        const auto savedMode = ConfigIni::GetInt(ConfigIni::Key::kAssistTick, 0);
        for (int mode : {0, 1, 2}) {
            ConfigIni::SetInt(ConfigIni::Key::kAssistTick, mode);
            ConfigIni::Save(); ConfigIni::Load();
            check(ConfigIni::GetInt(ConfigIni::Key::kAssistTick, -1) == mode,
                U"assist mode persists including legacy ON numeric value");
        }
        ConfigIni::SetInt(ConfigIni::Key::kAssistTick, savedMode);
        ConfigIni::Save();

        // Remove physical bindings so this check is independent of user keyboard activity.
        std::array<Input, KeyConfig::kConfigSetEnumCount> savedInputs;
        for (int i=0; i<KeyConfig::kConfigSetEnumCount; ++i) {
            const auto set = static_cast<KeyConfig::ConfigSet>(i);
            savedInputs[i] = KeyConfig::GetConfigValue(set, kButtonBT_A);
            KeyConfig::SetConfigValue(set, kButtonBT_A, Input{});
        }
        KeyConfig::RequestVirtualDown(kButtonBT_A); KeyConfig::Update();
        check(KeyConfig::Down(kButtonBT_A) && !KeyConfig::PhysicalDown(kButtonBT_A),
            U"virtual press remains available to UI but cannot produce an INPUT tick");
        KeyConfig::ClearInput(kButtonBT_A);
        for (int i=0; i<KeyConfig::kConfigSetEnumCount; ++i)
            KeyConfig::SetConfigValue(static_cast<KeyConfig::ConfigSet>(i), kButtonBT_A, savedInputs[i]);

        using BTDown = std::array<bool,kson::kNumBTLanesSZ>;
        using FXDown = std::array<bool,kson::kNumFXLanesSZ>;
        const auto capture = [&] {
            std::vector<float> pcm(8192);
            for (size_t i=0; i<pcm.size(); i+=256)
                if (render(128,pcm.data()+i)!=1024) throw Error{U"Incomplete INPUT render block"};
            return pcm;
        };
        const auto peak = [](const auto& pcm) {
            double value=0; for(float sample:pcm)value=std::max(value,std::abs(static_cast<double>(sample)));
            return value;
        };
        const auto record = [&](AssistTickMode mode, BTDown bt, FXDown fx, bool enabled=true) {
            AssistTick tick{mode};
            const auto start=count();
            tick.updateInput(bt,fx,enabled);
            check(count()==start,U"INPUT feedback does not enqueue chart-time events");
            return capture();
        };
        const auto bt = record(AssistTickMode::kInput, BTDown{true}, FXDown{});
        const auto fx = record(AssistTickMode::kInput, BTDown{}, FXDown{true});
        check(peak(bt)>0.001 && peak(fx)>0.001,U"BT and FX empty hits produce actual mixed PCM");
        check(peak(record(AssistTickMode::kOff,BTDown{true},FXDown{true}))<0.00001,
            U"OFF ignores physical button edges");
        check(peak(record(AssistTickMode::kAuto,BTDown{true},FXDown{true}))<0.00001,
            U"AUTO ignores physical button edges");
        check(peak(record(AssistTickMode::kInput,BTDown{},FXDown{}))<0.00001,
            U"held or released buttons without a new down edge do not play");
        check(peak(record(AssistTickMode::kInput,BTDown{true},FXDown{true},false))<0.00001,
            U"disabled gameplay feedback suppresses ticks during pause or exit");
        const auto chord = record(AssistTickMode::kInput,BTDown{true,true,true,true},FXDown{true,true});
        double chordError=0;
        for(size_t i=0;i<chord.size();++i)
            chordError=std::max(chordError,std::abs(static_cast<double>(chord[i])-4.0*bt[i]-2.0*fx[i]));
        check(chordError<0.0001,U"all six simultaneous button ticks mix without stealing voices");
        {
            AssistTick tick{AssistTickMode::kInput};
            tick.updateInput(BTDown{true},FXDown{},true); const auto first=capture();
            tick.updateInput(BTDown{true},FXDown{},true); const auto second=capture();
            check(peak(first)>0.001 && peak(second)>0.001,U"repeated down edges retrigger INPUT ticks");
            kson::ChartData notes=chart;notes.note.bt[0].emplace(1,kson::Interval{});
            context(90);const auto start=count();tick.update(notes,timing,0);
            check(count()==start,U"INPUT never creates automatic ticks even when chart notes exist");
        }
    }
    {
        for(kson::Pulse p:{1,2,3}) chart.note.bt[0].emplace(p,kson::Interval{});
        AssistTick tick{AssistTickMode::kAuto};context();const auto start=count();
        tick.update(chart,timing,0);check(count()-start==3,U"dense assist notes all scheduled in one frame");
        tick.update(chart,timing,0);check(count()-start==3,U"assist notes never queued twice");
        ksmaudio::CancelScheduledSounds();context(101);tick.update(chart,timing,0);
        check(count()-start==6,U"assist notes rearmed after seek generation change");
    }
    {
        for(kson::Pulse p:{1,2,3}) chart.audio.keySound.fx.chipEvent["timing-fixture.wav"][0].emplace(p,kson::KeySoundInvokeFX{});
        FXChipSE automatic{chart,timing,U".",true,1};GameStatus status;
        status.currentTimeSec=status.currentTimeSecForAudioProc=0;
        std::array<FXChipJudgedStatus,kson::kNumFXLanesSZ> judges{};
        context(200);const auto start=count();automatic.update(chart,status,judges);
        check(count()-start==3,U"dense automatic FX chips all scheduled");
        automatic.update(chart,status,judges);check(count()-start==3,U"automatic FX chips never queued twice");
    }
    {
        FXChipSE manual{chart,timing,U".",false,1};GameStatus status;
        status.currentTimeSec=status.currentTimeSecForAudioProc=0;
        std::array<FXChipJudgedStatus,kson::kNumFXLanesSZ> judges{};
        context(300);manual.update(chart,status,judges);pump();
        check(std::abs(buffer[0])<0.001f,U"manual FX never predicts an unjudged input");
        judges[0].lastJudgedChipPulse=1;judges[0].lastChipJudgedTimeSec=0.002;
        manual.update(chart,status,judges);pump();check(std::abs(buffer[0]-0.25f)<0.006f,U"judged FX input sounds on next block without timestamp wait");
    }
    {
        FXChipSE delayed{chart,timing,U".",false,1};GameStatus status;
        status.currentTimeSec=0.020;status.currentTimeSecForAudioProc=0;
        std::array<FXChipJudgedStatus,kson::kNumFXLanesSZ> judges{};
        judges[0].lastJudgedChipPulse=1;judges[0].lastChipJudgedTimeSec=0.020;
        context(400);const auto start=count();delayed.update(chart,status,judges);
        check(count()==start+1,U"positive manual delay schedules actual input feedback");pump();
        check(std::abs(buffer[0])<0.001f,U"explicit input delay is preserved");
    }
    {
        chart.audio.keySound.laser.slamEvent["timing-fixture.wav"]={1,2,3};
        chart.audio.keySound.laser.legacy.volAuto=true;
        for(size_t lane=0;lane<2;++lane)for(kson::Pulse p:{1,2,3})
            chart.note.laser[lane][0].v.emplace(p,kson::GraphPoint{kson::GraphValue{0,lane==0 ? 0.2 : 0.4}});
        LaserSlamSE automatic{chart,timing,U".",true,1};GameStatus status;
        status.currentTimeSec=status.currentTimeSecForAudioProc=0;context(500);const auto start=count();
        automatic.update(chart,status);check(count()-start==3,U"simultaneous dense slams choose louder lane once per pulse");
        automatic.update(chart,status);check(count()-start==3,U"automatic slams never queued twice");
    }
    {
        LaserSlamSE manual{chart,timing,U".",false,1};GameStatus status;
        status.currentTimeSec=status.currentTimeSecForAudioProc=0;context(600);
        status.laserLaneStatus[1].lastJudgedLaserSlamPulse=1;
        status.laserLaneStatus[1].lastLaserSlamJudgedTimeSec=0.002;
        manual.update(chart,status);pump();
        check(std::abs(buffer[0]-0.1f)<0.006f,U"judged slam input preserves volume without future note wait");
    }
    {
        const auto asioDriver=ksmaudio::GetAudioOutputStatus().driver;
        AssistTick input{AssistTickMode::kInput};
        for (auto backend : {ksmaudio::AudioBackend::Default, ksmaudio::AudioBackend::DirectSound}) {
            // Exercise actual Windows output APIs silently; only mock-ASIO PCM is audible to tests.
            ksmaudio::SetMute(true);
            const auto native=ksmaudio::ApplyOutputChange(s3d::Platform::Windows::Window::GetHWND(),{backend,{}});
            check(native.success && ksmaudio::GetAudioOutputStatus().backend==backend,
                U"INPUT sample resources rebuild for native Windows output");
            input.updateInput({true,true,true,true},{true,true},true);
            check(ksmaudio::GetLastErrorCode()==0,U"native output accepts immediate INPUT sample playback");
            const auto asio=ksmaudio::ApplyOutputChange(s3d::Platform::Windows::Window::GetHWND(),
                {ksmaudio::AudioBackend::ASIO,asioDriver});
            check(asio.success && ksmaudio::IsASIOActive(),U"INPUT resources rebuild back to ASIO");
            ksmaudio::SetMute(false);
            input.updateInput({true,true,true,true},{true,true},true);
            bool audible=false;
            for(int i=0;i<32;++i) {
                render(128,buffer.data());
                for(float sample:buffer)if(std::abs(sample)>0.001f)audible=true;
            }
            check(audible,U"INPUT remains audible after native to ASIO output change");
        }
    }
    ksmaudio::CancelScheduledSounds();report.writeln(U"TOTAL {} checks passed"_fmt(checks));
#else
    throw Error{U"Timing fixture requires Windows mock ASIO"};
#endif
}
}
