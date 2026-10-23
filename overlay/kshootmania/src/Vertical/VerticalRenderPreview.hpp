#pragma once
#include "MusicGame/Graphics/GraphicsMain.hpp"
#include "kson/kson.hpp"
#include "MusicGame/Camera/CamPattern/CamPatternMain.hpp"

// Render-only fixture: no GameMain, JudgmentMain, BGM, result scene or score
// persistence exists in this scene. It cannot establish judgment equivalence.
class VerticalRenderPreview : public Co::UpdaterSceneBase
{
    static FilePath chartPath()
    {
        const auto args = System::GetCommandLineArgs();
        for (size_t i = 0; i + 1 < args.size(); ++i)
            if (args[i] == U"--vertical-preview-chart") return args[i + 1];
        return U"";
    }
    static kson::ChartData chart()
    {
        const auto path = chartPath();
        if (!path.isEmpty()) return theme(FsUtils::HasKsonExtension(path)
            ? kson::LoadKsonChartData(path.toUTF8()) : kson::LoadKshChartData(path.toUTF8()));
        auto result = kson::CreateEditorDefaultChartData();
        result.meta.title = System::GetCommandLineArgs().contains(U"--vertical-preview-long-title")
            ? "A VERY LONG VERTICAL RENDER CHECK TITLE / Japanese 日本語 Korean 한글" : "VERTICAL RENDER CHECK";
        result.meta.artist = "Synthetic fixture / no score recording";
        result.meta.level = 15;
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-movie"))
            result.bg.legacy.movie.filename = "../../test-output/vertical-movie-fixture.avi";
        result.beat.bpm[0] = 180;
        for (int lane = 0; lane < 4; ++lane)
        {
            result.note.bt[lane][lane * kson::kResolution4 / 4] = kson::Interval{0};
            result.note.bt[lane][(lane + 4) * kson::kResolution4 / 4] = kson::Interval{kson::kResolution4 / 4};
        }
        result.note.fx[0][kson::kResolution4] = kson::Interval{0};
        result.note.fx[1][kson::kResolution4 * 3 / 2] = kson::Interval{kson::kResolution4 / 2};
        for (int lane = 0; lane < 2; ++lane)
        {
            kson::LaserSection section;
            section.w = System::GetCommandLineArgs().contains(U"--vertical-preview-wide") ? kson::kLaserXScale2x : kson::kLaserXScale1x;
            section.v[0] = lane == 0 ? 0.2 : 0.8;
            section.v[kson::kResolution4 / 2] = lane == 0 ? 0.8 : 0.2;
            section.v[kson::kResolution4] = kson::GraphValue{0.2, 0.8};
            section.v[kson::kResolution4 * 3 / 2] = 0.5;
            result.note.laser[lane][0] = section;
        }
        return theme(result);
    }
    static kson::ChartData theme(kson::ChartData result)
    {
        const auto args=System::GetCommandLineArgs();
        for(size_t i=0;i+1<args.size();++i)
        {
            if(args[i]==U"--vertical-preview-bg")
                for(auto& bg:result.bg.legacy.bg) bg.filename=args[i+1].toUTF8();
            if(args[i]==U"--vertical-preview-layer") result.bg.legacy.layer.filename=args[i+1].toUTF8();
        }
        return result;
    }
    static MusicGame::PlayOption options()
    {
        MusicGame::PlayOption result;
        result.showLayer = System::GetCommandLineArgs().contains(U"--vertical-preview-layer");
        result.movieEnabled = System::GetCommandLineArgs().contains(U"--vertical-preview-movie");
        result.diagnosticSession = true;
        const auto args=System::GetCommandLineArgs();for(size_t i=0;i+1<args.size();++i)
            if(args[i]==U"--vertical-preview-hispeed")result.hispeedSetting.value=Clamp(Parse<int>(args[i+1]),25,2000);
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-hard")) result.gaugeType = GaugeType::kHardGauge;
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-course")) result.gameMode = MusicGame::GameMode::kCourseMode;
        return result;
    }
    kson::ChartData m_chart = chart();
    kson::TimingCache m_timing = kson::CreateTimingCache(m_chart.beat);
    MusicGame::PlayOption m_options = options();
    MusicGame::GameStatus m_game;
    MusicGame::ViewStatus m_view;
    MusicGame::Scroll::HighwayScroll m_scroll{m_chart};
    MusicGame::Graphics::GraphicsMain m_graphics{m_chart, m_timing,
        chartPath().isEmpty() ? U"" : FileSystem::ParentPath(chartPath()), m_options};
    std::array<HashSet<kson::Pulse>, kson::kNumLaserLanesSZ> m_curved;
    int m_frames = 0;
    Stopwatch m_spinClock{StartImmediately::Yes};
public:
    VerticalRenderPreview() { m_graphics.prepareMovie(0); }
    ~VerticalRenderPreview() {m_graphics.writeWarpReport();}
    void update() override
    {
        m_game.currentBPM = m_chart.beat.bpm.at(0);
        const auto args=System::GetCommandLineArgs();
        for(size_t i=0;i+1<args.size();++i)
            if(args[i]==U"--vertical-preview-progress") {
                m_game.currentTimeSec=Clamp(Parse<double>(args[i+1])/100.0,0.0,1.0)*(m_options.movieEnabled ? 34.4 : 33.4);
                m_game.currentPulse=static_cast<kson::Pulse>(kson::SecToPulseDouble(m_game.currentTimeSec,m_chart.beat,m_timing));
            }
        m_view.score = 12345678;
        for(size_t i=0;i+1<args.size();++i)if(args[i]==U"--vertical-preview-score")m_view.score=Clamp(Parse<int>(args[i+1]),0,10000000);
        m_view.gaugePercentage = System::GetCommandLineArgs().contains(U"--vertical-preview-zero") ? 0 :
            (System::GetCommandLineArgs().contains(U"--vertical-preview-hundred") ? 100 : 70);
        m_view.gaugePercentageInt = static_cast<int>(m_view.gaugePercentage);
        m_view.effectiveGaugeType = m_options.gaugeType;
        if(args.contains(U"--aps-visual-fixture")) {
            if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"APS capture requires isolated fixture"};
            m_view.effectiveGaugeType = args.contains(U"--aps-spent") ? GaugeType::kNormalGauge : GaugeType::kHardGauge;
            m_view.apsProtected = !args.contains(U"--aps-spent");
        }
        if(args.contains(U"--aps-performance-fixture")) {
            if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"audio-test-fixture")))throw Error{U"APS benchmark requires isolated fixture"};
            m_view.effectiveGaugeType=GaugeType::kNormalGauge;m_view.apsProtected=false;
            m_view.gaugePercentage=70;m_view.gaugePercentageInt=70;
            m_view.gaugeVisual={true,70,0,args.contains(U"--aps-performance-hidden")?0.0:1.0,0};
            if(args.contains(U"--aps-performance-overlap")) {
                MusicGame::GaugePresentation state;state.update(70,true,false,false,0,0);
                state.update(70,false,true,false,0,.1);
                m_view.gaugeVisual=state.update(70,false,false,false,.6,.7);
            }
        }
        ++m_frames;
        m_view.displayCombo = 128;
        m_game.laserLaneStatus[0].cursorX = 0.25;
        m_game.laserLaneStatus[1].cursorX = 0.75;
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-effects"))
        {
            m_game.btLaneStatus[0].keyBeamTimeSec = -0.09;
            m_game.btLaneStatus[0].chipAnim.push({-0.12, MusicGame::Judgment::ChipAnimType::kCritical});
            m_game.btLaneStatus[2].keyBeamTimeSec = -0.09;
            m_game.btLaneStatus[2].longNotePressed = true;
            m_game.btLaneStatus[2].currentLongNotePulse = 0;
            m_game.btLaneStatus[2].currentLongNoteAnimOffsetTimeSec = -0.12;
            m_game.fxLaneStatus[1].keyBeamTimeSec = -0.09;
            m_game.fxLaneStatus[1].chipAnim.push({-0.12, MusicGame::Judgment::ChipAnimType::kNearFast});
            for (auto& lane : m_game.laserLaneStatus)
            {
                lane.noteCursorX = lane.cursorX;
                lane.currentLaserSectionPulse = 0;
                lane.rippleAnim.push({-0.06, lane.cursorWide, lane.cursorX.value_or(0.5)});
            }
        }
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-wide"))
            for (auto& lane : m_game.laserLaneStatus) lane.cursorWide = true;
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-motion"))
        {
            m_view.camStatus.zoomBottom = 0.15;
            m_view.camStatus.zoomTop = -0.10;
            m_view.camStatus.shiftX = 0.2;
            m_view.camStatus.rotationDeg = 8;
            m_view.camStatus.rotationDegHighway = -3;
            m_view.camStatus.rotationDegJdgline = -3;
            m_view.camStatus.centerSplit = 0.12;
            m_view.tiltRadians = 0.08;
            m_view.tiltRadiansForBgLayer = 0.08;
            m_view.camStatus.useLegacyZoomTop = System::GetCommandLineArgs().contains(U"--vertical-preview-legacy");
            m_view.camStatus.useLegacyJdgScale = m_view.camStatus.useLegacyZoomTop;
        }
        for (size_t i=0; i+1<args.size(); ++i)
            if (args[i]==U"--vertical-preview-tilt") {
                // Diagnostic input in degrees, not a chart or player-input conversion.
                m_view.tiltRadians = Math::ToRadians(Parse<double>(args[i+1]));
                m_view.tiltRadiansForBgLayer = m_view.tiltRadians;
            }
        for(size_t i=0;i+1<args.size();++i) {
            if(args[i]==U"--vertical-preview-zoom-bottom") m_view.camStatus.zoomBottom=Parse<double>(args[i+1]);
            if(args[i]==U"--vertical-preview-zoom-top") m_view.camStatus.zoomTop=Parse<double>(args[i+1]);
            if(args[i]==U"--vertical-preview-shift") m_view.camStatus.shiftX=Parse<double>(args[i+1]);
            if(args[i]==U"--vertical-preview-spin") m_view.camStatus.rotationDeg=Parse<double>(args[i+1]);
        }
        if(args.contains(U"--vertical-preview-warp-cycle")) {
            m_view.camStatus.zoomTop=110*Sin(m_frames*0.025);
        }
        if(args.contains(U"--vertical-preview-bench-cycle")) {
            m_view.camStatus.zoomTop=100*Sin(m_frames*.025);
            m_view.camStatus.zoomBottom=100*Cos(m_frames*.017);
            m_view.tiltRadians=Math::ToRadians(10*Sin(m_frames*.013));
            m_view.tiltRadiansForBgLayer=m_view.tiltRadians;
            m_view.camStatus.rotationDeg=20*Sin(m_frames*.009);
            m_view.camStatus.rotationDegLayer=m_view.camStatus.rotationDeg;
        }
        if (args.contains(U"--vertical-preview-layer-static")) {
            m_chart.bg.legacy.layer.rotation.tilt = false;
            m_chart.bg.legacy.layer.rotation.spin = false;
        }
        if (System::GetCommandLineArgs().contains(U"--vertical-preview-clear") || System::GetCommandLineArgs().contains(U"--vertical-preview-fail"))
            m_game.playFinishStatus = MusicGame::PlayFinishStatus{.finishTimeSec = -2,
                .achievement = System::GetCommandLineArgs().contains(U"--vertical-preview-clear") ? Achievement::kCleared : Achievement::kNone};
        m_scroll.update(m_options.hispeedSetting, m_game.currentBPM);
        for(size_t i=0;i+1<args.size();++i) if(args[i]==U"--vertical-preview-event-rate" && !m_chart.camera.cam.pattern.laser.slamEvent.spin.empty()) {
            auto it=m_chart.camera.cam.pattern.laser.slamEvent.spin.begin();
            for(size_t j=0;j+1<args.size();++j)if(args[j]==U"--vertical-preview-event-index")std::advance(it,Clamp(Parse<int>(args[j+1]),0,static_cast<int>(m_chart.camera.cam.pattern.laser.slamEvent.spin.size())-1));
            const auto& event=*it;
            m_view.displayCombo=m_frames==1 ? 0 : 128;
            MusicGame::Camera::CamPatternMain pattern{m_chart};
            pattern.onLaserSlamJudged(event.first,event.second.d,event.first);
            m_view.camStatus.rotationDegHighway=0;m_view.camStatus.rotationDegJdgline=0;m_view.camStatus.rotationDegLayer=0;
            pattern.applyToCamStatus(m_view.camStatus,event.first+static_cast<kson::Pulse>(event.second.length*Parse<double>(args[i+1])));
            m_game.currentTimeSec=kson::PulseToSec(event.first,m_chart.beat,m_timing);
            m_game.currentPulse=event.first;
            if(args.contains(U"--vertical-preview-event-cycle")) {
                m_view.camStatus.rotationDegHighway=0;m_view.camStatus.rotationDegJdgline=0;m_view.camStatus.rotationDegLayer=0;
                const double rate=Clamp((m_spinClock.sF()-.1)/2.0,0.0,1.5);
                pattern.applyToCamStatus(m_view.camStatus,event.first+static_cast<kson::Pulse>(event.second.length*rate));
            }
        }
        m_graphics.update(m_game, m_view, m_timing);
    }
    void draw() const override
    {
        const MusicGame::Scroll::HighwayScrollContext context{&m_scroll, &m_chart.beat, &m_timing, &m_game};
        m_graphics.draw(m_chart, m_curved, m_timing, m_game, m_view, context, 30s);
    }
};
