#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "MusicGame/Judgment/RecentTimingWindow.hpp"
#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

using MusicGame::Judgment::RecentTimingWindow;

TEST_CASE("An empty window has no fabricated measurement")
{
	RecentTimingWindow window;
	REQUIRE(window.empty());
	REQUIRE_FALSE(window.mean());
	window.expire(100, 10);
	REQUIRE_FALSE(window.mean());
}

TEST_CASE("The ten second boundary expires inclusively")
{
	RecentTimingWindow window;
	window.add(0, 10);
	window.add(1, -30);
	window.expire(9.999, 10);
	REQUIRE(window.size() == 2);
	REQUIRE(window.mean().value() == -10);
	window.expire(10, 10);
	REQUIRE(window.size() == 1);
	REQUIRE(window.mean().value() == -30);
	window.expire(11, 10);
	REQUIRE(window.empty());
	REQUIRE_FALSE(window.mean());
}

TEST_CASE("Zero error is a valid sample and clear removes old calibration")
{
	RecentTimingWindow window;
	window.add(0, 0);
	REQUIRE(window.mean().has_value());
	REQUIRE(window.mean().value() == 0);
	window.clear();
	window.add(1, 25);
	REQUIRE(window.size() == 1);
	REQUIRE(window.mean().value() == 25);
}

TEST_CASE("Incremental production statistics match an independent filtered history")
{
	struct Measurement
	{
		double timeSeconds;
		double offsetMs;
	};
	std::vector<Measurement> history;
	std::mt19937 random(0x4B534D);
	std::uniform_int_distribution<int> error(-150, 150);
	RecentTimingWindow window;
	for (int frame = 0; frame < 20000; ++frame)
	{
		const double nowSeconds = frame * 0.05;
		if (frame % 3 != 0)
		{
			const double offsetMs = error(random);
			history.push_back({ nowSeconds, offsetMs });
			window.add(nowSeconds, offsetMs);
		}
		window.expire(nowSeconds, 10);
		double totalMs = 0;
		std::size_t count = 0;
		for (const auto& sample : history)
		{
			if (sample.timeSeconds > nowSeconds - 10)
			{
				totalMs += sample.offsetMs;
				++count;
			}
		}
		REQUIRE(window.size() == count);
		if (count)
			REQUIRE(window.mean().value() == Approx(totalMs / count));
		else
			REQUIRE_FALSE(window.mean());
	}
}
