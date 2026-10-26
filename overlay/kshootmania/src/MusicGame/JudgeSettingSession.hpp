#pragma once
#include "TimingFeedback.hpp"
#include "Judgment/InputTimingStats.hpp"
#include "Judgment/DetailedJudgmentStats.hpp"
#include "Judgment/RecentTimingWindow.hpp"

namespace MusicGame
{
	// A live draft belongs to one settings screen. Normal games never receive it.
	struct JudgeSettingValues
	{
		FastSlowMode mode = FastSlowMode::kHide;
		TimingHudSettings hud;
		int32 input = 0, visual = 0, laser = 0;
		bool operator==(const JudgeSettingValues& v) const
		{
			return mode == v.mode && hud.judgeColor == v.hud.judgeColor && hud.position == v.hud.position && hud.display == v.hud.display && hud.color == v.hud.color && input == v.input && visual == v.visual && laser == v.laser;
		}
	};
	struct JudgeSettingSession
	{
		static constexpr double kLoopSeconds = 16.0, kRecentSeconds = 10.0, kGuardSeconds = .2;
		JudgeSettingValues values;
		bool menu = false, focused = true, rebase = false;
		uint64 loops = 0, measurementRevision = 0;
		double time = 0;
		Judgment::DetailedJudgmentStats counts;
		Judgment::InputTimingStats cumulative;
		Judgment::RecentTimingWindow recent;
		Optional<double> last, pendingLast, recentDisplay;
		bool enabled() const
		{
			return !menu && focused;
		}
		void clearRecent();
		void clearMeasurements();
		void advance(double delta);
		void chip(Judgment::JudgmentResult result, Optional<double> ms);
		void tick(Judgment::JudgmentResult result);
		void finishFrame();
		Optional<double> recentMean() const
		{
			return recentDisplay;
		}
		bool itemEnabled(int32 index) const
		{
			return index == 0 || index >= 5 || values.mode == FastSlowMode::kNew;
		}
		int32 navigate(int32 index, int32 delta) const;

		void change(int32 index, int32 delta);
	};
}
