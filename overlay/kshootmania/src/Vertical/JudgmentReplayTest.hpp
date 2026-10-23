#pragma once
#include "DiagnosticInputReplay.hpp"
#include "VerticalModConfig.hpp"
#include "MusicGame/Judgment/JudgmentMain.hpp"

namespace Vertical
{
    struct ReplayRun
    {
        MusicGame::PlayResult result;
        Array<String> transitions;
        Array<String> laserFrames;
        int laserCorrectFrames = 0;
        int laserOutsideFrames = 0;
    };

    inline ReplayRun ReplayJudgment(int bpm, int fps, bool verticalEnabled, bool withLaser = false)
    {
        using namespace MusicGame;
        auto config = ModConfig::Load();
        config.enabled = verticalEnabled;
        config.Save(); // In-memory only; no ConfigIni::Save in this test.
        auto chart = kson::CreateEditorDefaultChartData();
        chart.beat.bpm[0] = bpm;
        struct Event { double start, end; int button; };
        Array<Event> events;
        for (int n = 0; n < 64; ++n)
        {
            const auto pulse = static_cast<kson::Pulse>((n + 8) * kson::kResolution4 / 4);
            const bool isLong = n % 13 == 0;
            const auto length = isLong ? kson::kResolution4 / 2 : 0;
            const int lane = n % 6;
            if (lane < 4) chart.note.bt[lane][pulse] = kson::Interval{length};
            else chart.note.fx[lane - 4][pulse] = kson::Interval{length};
            if (n % 11 == 0) continue; // Deliberate MISS, not autoplay.
            const double time = (n + 8) * 60.0 / bpm;
            const double offset = (n % 5 - 2) * 0.04;
            events.push_back({time + offset, time + offset + (isLong ? 120.0 / bpm : 0.06), lane});
        }
        const auto timing = kson::CreateTimingCache(chart.beat);
        if (withLaser)
        {
            for (int n = 0; n < 8; ++n)
            {
                kson::LaserSection section;
                section.w = n % 2 == 0 ? kson::kLaserXScale1x : kson::kLaserXScale2x;
                section.v[0] = 0.15;
                section.v[kson::kResolution4 / 4] = 0.85;
                section.v[kson::kResolution4 / 2] = kson::GraphValue{0.85, 0.15};
                section.v[kson::kResolution4 * 3 / 4] = 0.60;
                section.v[kson::kResolution4] = 0.60;
                chart.note.laser[n % 2][(12 + n * 6) * kson::kResolution4 / 4] = section;
            }
        }
        PlayOption options;
        options.diagnosticSession = true;
        options.inputDelayMs = 7;
        options.visualOffsetMs = -3;
        options.laserInputDelayMs = -11;
        options.globalOffsetMs = 19;
        options.audioProcDelayMs = 5;
        Judgment::JudgmentMain judgment{chart, timing, options, none, GameMode::kNormal};
        GameStatus game;
        ViewStatus view;
        DiagnosticInputReplay::Session session;
        ReplayRun run;
        int previousScore = -1, previousCombo = -1;
        const double duration = 78 * 60.0 / bpm;
        for (int frame = 0; frame <= static_cast<int>(duration * fps); ++frame)
        {
            const double time = frame / static_cast<double>(fps);
            uint32_t held = 0;
            for (const auto event : events)
                if (time >= event.start && time < event.end) held |= DiagnosticInputReplay::Mask(event.button);
            std::array<double, 2> delta{};
            if (withLaser)
            {
                // Physical input schedule, independent of the judgment cursor.
                // Include opposite-direction sections to exercise misses.
                const auto position = [](double beat) {
                    if (beat < 0) return 0.15;
                    if (beat < 1) return 0.15 + beat * 0.70;
                    if (beat < 2) return 0.85;
                    if (beat < 3) return 0.15 + (beat - 2) * 0.45;
                    return 0.60;
                };
                for (int n = 0; n < 8; ++n)
                {
                    const double beat = time * bpm / 60.0 - (12 + n * 6);
                    const double previousBeat = (time - 1.0 / fps) * bpm / 60.0 - (12 + n * 6);
                    if (beat < 0 || previousBeat >= 4) continue;
                    delta[n % 2] += (position(beat) - position(previousBeat)) * (n % 3 == 2 ? -1.0 : 1.0);
                }
            }
            DiagnosticInputReplay::Frame(held, delta);
            game.currentTimeSec = time;
            game.currentBPM = bpm;
            game.currentTimeSecForButtonJudgment = time - options.effectiveInputDelayMs() / 1000.0;
            game.currentTimeSecForLaserJudgment = game.currentTimeSecForButtonJudgment - options.effectiveLaserInputDelayMs() / 1000.0;
            game.currentPulse = kson::SecToPulse(time, chart.beat, timing);
            game.currentPulseForButtonJudgment = kson::SecToPulse(game.currentTimeSecForButtonJudgment, chart.beat, timing);
            game.currentPulseForLaserJudgment = kson::SecToPulse(game.currentTimeSecForLaserJudgment, chart.beat, timing);
            judgment.update(chart, game, view);
            if (withLaser)
            {
                for (int lane = 0; lane < 2; ++lane)
                {
                    const auto& laser = game.laserLaneStatus[lane];
                    if (laser.noteCursorX)
                    {
                        if (laser.isCursorInCriticalJudgmentRange()) ++run.laserCorrectFrames;
                        else ++run.laserOutsideFrames;
                    }
                    run.laserFrames.push_back(U"{},{},{:.9f},{:.9f},{:.9f},{:.9f},{},{},{}"_fmt(frame, lane,
                        delta[lane], laser.cursorX.value_or(-1.0), laser.noteCursorX.value_or(-1.0),
                        laser.noteVisualCursorX.value_or(-1.0), laser.cursorWide, laser.lastJudgedLaserSlamPulse,
                        laser.isCursorInCriticalJudgmentRange()));
                }
            }
            if (view.score != previousScore || view.displayCombo != previousCombo)
            {
                run.transitions.push_back(U"{},{},{},{:.8f}"_fmt(frame, view.score, view.displayCombo, view.gaugePercentage));
                previousScore = view.score;
                previousCombo = view.displayCombo;
            }
        }
        run.result = judgment.playResult(chart, timing, duration, IsHardFailedYN::No, false);
        return run;
    }

    inline void RunJudgmentReplayTest()
    {
        ConfigIni::Load();
        const auto original = ModConfig::Load();
        TextWriter report{U"vertical-judgment-replay.csv"};
        report.writeln(U"fixture,bpm,fps,score,critical,near_fast,near_slow,error,max_combo,gauge_value,transitions,laser_correct_frames,laser_outside_frames,status");
        bool allPassed = true;
        for (const bool withLaser : {false, true})
        for (const int bpm : {120, 180, 240})
        {
            for (const int fps : {60, 120, 144, 240})
            {
                const auto horizontal = ReplayJudgment(bpm, fps, false, withLaser);
                const auto vertical = ReplayJudgment(bpm, fps, true, withLaser);
                const auto& a = horizontal.result;
                const auto& b = vertical.result;
                const auto& sa = a.comboStats;
                const auto& sb = b.comboStats;
                const bool ok = a.score == b.score && a.maxCombo == b.maxCombo && a.gaugeValue == b.gaugeValue &&
                    a.uncappedGaugeValue == b.uncappedGaugeValue && a.totalCombo == b.totalCombo &&
                    sa.critical == sb.critical && sa.nearFast == sb.nearFast && sa.nearSlow == sb.nearSlow && sa.error == sb.error &&
                    horizontal.transitions == vertical.transitions && sa.critical > 0 && sa.error > 0 &&
                    sa.totalNear() > 0 && !a.playOption.shouldSaveScore() && !b.playOption.shouldSaveScore() &&
                    horizontal.laserFrames == vertical.laserFrames &&
                    (!withLaser || (horizontal.laserCorrectFrames > 0 && horizontal.laserOutsideFrames > 0));
                allPassed = allPassed && ok;
                report.writeln(U"{},{},{},{},{},{},{},{},{},{},{},{},{},{}"_fmt(withLaser ? U"BT_FX_LASER" : U"BT_FX",
                    bpm, fps, a.score, sa.critical, sa.nearFast, sa.nearSlow, sa.error, a.maxCombo, a.gaugeValue,
                    horizontal.transitions.size(), horizontal.laserCorrectFrames, horizontal.laserOutsideFrames, ok ? U"PASS" : U"FAIL"));
                if (withLaser && bpm == 180 && fps == 120)
                {
                    TextWriter trace{U"vertical-laser-replay.csv"};
                    trace.writeln(U"frame,lane,input_delta,cursor_x,note_x,visual_note_x,wide,last_slam_pulse,in_critical_range");
                    for (const auto& row : vertical.laserFrames) trace.writeln(row);
                }
            }
        }
        original.Save();
        report.writeln(U"overall,,,,,,,,,,,,,{}"_fmt(allPassed ? U"PASS" : U"FAIL"));
    }
}
