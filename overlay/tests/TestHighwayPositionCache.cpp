#include "MusicGame/Scroll/HighwayScroll.hpp"
#include <catch2/catch.hpp>
using namespace MusicGame;
using namespace MusicGame::Scroll;
TEST_CASE("Frame position cache equals original integer calculation across collisions and state changes", "[HighwayScroll][Cache]")
{
	kson::ChartData chart;
	chart.beat.bpm[0] = 120;
	chart.beat.bpm[1920] = 190;
	chart.beat.scrollSpeed[0] = kson::GraphPoint{ 1.0 };
	chart.beat.scrollSpeed[960] = kson::GraphPoint{ -2.0 };
	chart.beat.scrollSpeed[1440] = kson::GraphPoint{ 0.0 };
	chart.beat.scrollSpeed[1920] = kson::GraphPoint{ 1.5 };
	chart.beat.stop[720] = 120;
	auto timing = kson::CreateTimingCache(chart.beat);
	HighwayScroll scroll(chart);
	GameStatus state{};
	const HighwayScrollContext context(&scroll, &chart.beat, &timing, &state);
	for (const auto type : { HispeedType::OMod, HispeedType::XMod, HispeedType::CMod })
		for (int speed : { 450, 850 })
		{
			HispeedSetting setting;
			setting.type = type;
			setting.value = speed;
			scroll.update(setting, 120);
			for (double current : { -120.25, 0., 720.5, 1440., 2500.75 })
			{
				state.currentPulseDouble = current;
				state.currentPulse = int(current);
				state.currentTimeSec = kson::PulseToSec(int(current), chart.beat, timing);
				for (int repeat = 0; repeat < 2; ++repeat)
					for (kson::Pulse pulse = -3000; pulse < 5000; pulse += 7)
						REQUIRE(context.getPositionY(pulse) == scroll.getPositionY(pulse, chart.beat, timing, state));
				state.currentTimeSec += .001;
				REQUIRE(context.getPositionY(960) == scroll.getPositionY(960, chart.beat, timing, state));
			}
		}
}
