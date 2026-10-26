#include "NormalGaugeState.hpp"

namespace MusicGame::Judgment
{
	void NormalGaugeState::process(JudgmentResult result, bool chip, int32 maximum)
	{
		if (result == JudgmentResult::kError)
		{
			const int32 sub = Decrease(ErrorBase(maximum, chip));
			value = Max(0, value - sub);
			uncapped -= sub;
		}
		else
		{
			const int32 add = Increase(chip ? (result == JudgmentResult::kCritical ? kGaugeValueChip : kGaugeValueChipNear) : kGaugeValueLong);
			value = Min(maximum, value + add);
			uncapped += add;
		}
	}
}
