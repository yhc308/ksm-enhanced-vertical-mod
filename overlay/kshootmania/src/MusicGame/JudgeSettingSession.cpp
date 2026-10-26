#include "JudgeSettingSession.hpp"

namespace MusicGame
{
	void JudgeSettingSession::clearRecent()
	{
		recent.clear();
		pendingLast.reset();
	}

	void JudgeSettingSession::clearMeasurements()
	{
		clearRecent();
		counts = {};
		cumulative = {};
		++measurementRevision;
	}

	void JudgeSettingSession::advance(double delta)
	{
		if (focused)
			time += Max(0.0, delta);
		if (!menu)
			recent.expire(time, kRecentSeconds);
		pendingLast.reset();
	}

	void JudgeSettingSession::chip(Judgment::JudgmentResult result, Optional<double> ms)
	{
		if (!enabled())
			return;
		counts.add(result, ms);
		cumulative.add(result, ms);
		if (!ms || !std::isfinite(*ms))
			return;
		if (!pendingLast || Abs(*ms) > Abs(*pendingLast))
			pendingLast = *ms;
		if (result == Judgment::JudgmentResult::kCritical || IsTimingNear(result))
		{
			recent.add(time, *ms);
		}
	}

	void JudgeSettingSession::tick(Judgment::JudgmentResult result)
	{
		if (enabled())
			counts.add(result);
	}

	void JudgeSettingSession::finishFrame()
	{
		if (pendingLast)
		{
			last = pendingLast;
			const auto mean = recent.mean();
			recentDisplay = mean ? Optional<double>{ *mean } : none;
		}
	}

	int32 JudgeSettingSession::navigate(int32 index, int32 delta) const
	{
		constexpr std::array<int32, 10> order{ 1, 2, 3, 4, 0, 5, 6, 7, 8, 9 };
		int32 position = 0;
		for (int32 i = 0; i < 10; ++i)
			if (order[i] == index)
				position = i;
		const int32 direction = Sign(delta);
		for (int32 n = 0; n < Abs(delta); ++n)
		{
			int32 next = position + direction;
			while (next >= 0 && next < 10 && !itemEnabled(order[next]))
				next += direction;
			if (next < 0 || next >= 10)
				break;
			position = next;
		}
		return order[position];
	}

	void JudgeSettingSession::change(int32 index, int32 delta)
	{
		if (!itemEnabled(index) || !delta)
			return;
		const auto edit = [delta](auto& value, int32 maximum)
		{
			using T = std::decay_t<decltype(value)>;
			value = static_cast<T>(Clamp(static_cast<int32>(value) + delta, 0, maximum));
		};
		const auto before = values;
		if (index == 0)
			edit(values.mode, 2);
		if (index == 1)
			edit(values.hud.judgeColor, 1);
		if (index == 2)
			edit(values.hud.color, 3);
		if (index == 3)
			values.hud.display = TimingDisplayChoice(TimingDisplayCursor(values.hud.display) + delta);
		if (index == 4)
			edit(values.hud.position, 3);
		if (index >= 5 && index <= 7)
		{
			auto& value = index == 5 ? values.input : index == 6 ? values.visual
																 : values.laser;
			value = Clamp(value + delta, -10000, 10000);
		}
		if (before.input != values.input || before.visual != values.visual || before.laser != values.laser)
		{
			clearMeasurements();
			rebase = true;
		}
	}
}
