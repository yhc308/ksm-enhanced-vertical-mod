#pragma once
#include "MusicGame/GameDefines.hpp"
#include "JudgmentDefines.hpp"

namespace MusicGame::Judgment
{
	// Shared with the ordinary NORMAL path, including both integer truncations.
	struct NormalGaugeState
	{
		int32 value = 0;
		int32 uncapped = 0;
		static int32 Increase(int32 add)
		{
			return static_cast<int32>(add * kGaugeIncreaseRateNormal);
		}
		static int32 Decrease(int32 sub)
		{
			return static_cast<int32>(sub * kGaugeDecreaseRateNormal);
		}
		static int32 ErrorBase(int32 maximum, bool chip)
		{
			return static_cast<int32>(maximum * (chip ? kGaugeDecreasePercentByChipError : kGaugeDecreasePercentByLongError) / 100);
		}
		void process(JudgmentResult result, bool chip, int32 maximum);
	};
}
