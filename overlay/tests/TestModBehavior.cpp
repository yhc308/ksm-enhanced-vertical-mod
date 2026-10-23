#include <catch2/catch.hpp>
#include "MusicGame/Judgment/ExScore.hpp"
#include "MusicGame/Judgment/NormalGaugeState.hpp"
#include "MusicGame/GaugePresentation.hpp"
#include "MusicGame/JudgeSettingSession.hpp"
#include "MusicGame/TimingFeedback.hpp"
#include <limits>
#include "HighScore/AdditionalScoreIO.hpp"

using namespace MusicGame;
using namespace MusicGame::Judgment;

TEST_CASE("EX chip scoring preserves inclusive P-CRITICAL boundaries", "[EX][Boundary]")
{
	for (const double offsetMs : { -21.0, 0.0, 21.0 })
	{
		REQUIRE(ExChipPoints(JudgmentResult::kCritical, offsetMs) == 5);
	}
	REQUIRE(ExChipPoints(JudgmentResult::kCritical, 21.001) == 4);
	REQUIRE(ExChipPoints(JudgmentResult::kNearFast, 75.0) == 2);
	REQUIRE(ExTickPoints(JudgmentResult::kCritical) == 2);
	REQUIRE(ExTickPoints(JudgmentResult::kError) == 0);
	REQUIRE(ExPanelKind(9800, 10000) == 1);
	REQUIRE(ExPanelKind(9801, 10000) == 2);
	REQUIRE(ExPanelKind(10000, 10000) == 3);
	REQUIRE(ExPercentHundredths(97999, 100000) == 9799);
}

TEST_CASE("NORMAL gauge stays bounded through sustained misses and recovery", "[Gauge]")
{
	NormalGaugeState gauge;
	for (int i = 0; i < 10000; ++i)
	{
		gauge.process(JudgmentResult::kCritical, true, 10000);
	}
	REQUIRE(gauge.value == 10000);
	REQUIRE(gauge.uncapped > gauge.value);
	for (int i = 0; i < 100000; ++i)
	{
		gauge.process(JudgmentResult::kError, true, 10000);
	}
	REQUIRE(gauge.value == 0);
	REQUIRE(gauge.uncapped < 0);
	gauge.process(JudgmentResult::kCritical, true, 10000);
	REQUIRE(gauge.value > 0);
}

TEST_CASE("APS visual transition follows live NORMAL target without changing it", "[APS][Presentation]")
{
	GaugePresentation presentation;
	presentation.update(0, true, false, false, 0, 5);
	auto visual = presentation.update(80, false, true, false, 0, 5);
	REQUIRE(visual.percent == 0);
	visual = presentation.update(60, false, false, false, 0.25, 5.25);
	REQUIRE(visual.percent == Approx(36.5625));
	const auto paused = presentation.update(60, false, false, true, 5.0, 5.25);
	REQUIRE(paused.percent == visual.percent);
	visual = presentation.update(25, false, false, false, 2.0, 7.25);
	REQUIRE(visual.percent == 25);
	REQUIRE(visual.bannerOpacity == 0);
	visual = presentation.update(100, true, false, false, 0, 0);
	REQUIRE(visual.percent == 100);
	REQUIRE(visual.bannerOpacity == 1);
}

TEST_CASE("Judge settings retain displayed measurements until real chip input", "[JudgeSetting][Statistics]")
{
	JudgeSettingSession session;
	session.chip(JudgmentResult::kCritical, 10.0);
	session.finishFrame();
	REQUIRE(session.recentMean().value() == 10.0);
	session.advance(11.0);
	session.finishFrame();
	REQUIRE(session.recentMean().value() == 10.0);
	REQUIRE(session.last.value() == 10.0);
	session.chip(JudgmentResult::kCritical, -20.0);
	session.finishFrame();
	REQUIRE(session.recentMean().value() == -20.0);
	REQUIRE(session.cumulative.count == 2);
	session.menu = true;
	session.chip(JudgmentResult::kCritical, 1.0);
	session.finishFrame();
	REQUIRE(session.cumulative.count == 2);
}

TEST_CASE("Timing feedback chooses the largest simultaneous input once", "[Timing]")
{
	TimingFeedbackState state;
	state.offer(JudgmentResult::kCritical, 10.0);
	state.offer(JudgmentResult::kNearFast, 80.0);
	state.offer(JudgmentResult::kNearSlow, -80.0);
	const auto result = state.commit(1.0);
	REQUIRE(result->signedMs == 80.0);
	REQUIRE(state.commit(2.0)->displayedAt == 1.0);
	state.offer(JudgmentResult::kCritical, std::numeric_limits<double>::quiet_NaN());
	REQUIRE(state.commit(3.0)->signedMs == 80.0);
}

TEST_CASE("Sixteen ms color setting excludes its exact boundary", "[Timing][Compatibility]")
{
	TimingHudSettings settings;
	settings.color = TimingMsColor::kSixteenMs;
	settings.display = TimingMsDisplay::kAll;
	TimingFeedback event{ JudgmentResult::kCritical, 16.0, 0 };
	REQUIRE(PresentTiming(settings, event, 0).color == Color(178, 178, 178));
	event.signedMs = 16.001;
	REQUIRE(PresentTiming(settings, event, 0).color == JudgeDirectionColor(settings.judgeColor, true));
}

TEST_CASE("Additional EX records preserve legacy and optional maximum compatibility", "[EX][Persistence]")
{
	const String prefix = U"normal,normal,x10,on,on,on=100,0,1,0,1,1,0,0,0,";
	for (const String tail : { U"95", U"95,100", U"95,", U"95,invalid" })
	{
		const auto record = AdditionalScoreIO::ParseRecord(prefix + tail);
		REQUIRE(record.has_value());
		REQUIRE(record->first == U"normal,normal,x10,on,on,on");
		REQUIRE(record->second == 95);
	}
	REQUIRE(AdditionalScoreIO::ParseRecord(prefix + U"0,100")->second == 0);
	REQUIRE_FALSE(AdditionalScoreIO::ParseRecord(prefix + U"-1,100").has_value());
	REQUIRE_FALSE(AdditionalScoreIO::ParseRecord(prefix + U",100").has_value());
	REQUIRE_FALSE(AdditionalScoreIO::ParseRecord(prefix + U"95,100,extra").has_value());
}
