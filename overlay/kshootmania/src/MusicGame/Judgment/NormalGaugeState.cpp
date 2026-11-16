#include "NormalGaugeState.hpp"

namespace MusicGame::Judgment
{
	void NormalGaugeState::process(JudgmentResult result, bool chip, int32 maximum)
	{
		if (result == JudgmentResult::kError)
		{
			// Match 2.2.0 NORMAL: apply all factors before the single integer truncation.
			const double percent = chip ? kGaugeDecreasePercentByChipError : kGaugeDecreasePercentByLongError;
			const int32 sub = static_cast<int32>(maximum * percent * kGaugeDecreaseRateNormal / 100);
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
