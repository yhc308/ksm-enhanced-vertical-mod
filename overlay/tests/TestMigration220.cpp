#include <catch2/catch.hpp>
#include "MusicGame/Judgment/ScoringStatus.hpp"
#include "NocoUI/Asset.hpp"
#include "Scenes/Select/SelectMenuCacheDb.hpp"

using namespace MusicGame;
using namespace MusicGame::Judgment;

TEST_CASE("APS shadow NORMAL follows 2.2.0 single-truncation gauge rules", "[Migration220][APS]")
{
	for (const int maximum : { 997, 1000, 10001, 123457 })
	{
		ScoringStatus normal{ 10000000, maximum, GaugeType::kNormalGauge, none, GameMode::kNormal };
		ScoringStatus aps{ 10000000, maximum, GaugeType::kHardGauge, none, GameMode::kNormal, true };
		for (int i = 0; i < 2000; ++i)
		{
			const auto result = i % 13 == 0 ? JudgmentResult::kError
				: i % 7 == 0 ? JudgmentResult::kNearFast : JudgmentResult::kCritical;
			if (i % 3 == 0)
			{
				// Long/laser-line ticks have no NEAR judgment.
				const auto tick = result == JudgmentResult::kError ? result : JudgmentResult::kCritical;
				normal.onLongOrLaserLineJudgment(tick);
				aps.onLongOrLaserLineJudgment(tick);
			}
			else
			{
				normal.onChipOrLaserSlamJudgment(result);
				aps.onChipOrLaserSlamJudgment(result);
			}
			REQUIRE(aps.apsNormal().value == normal.gaugeValue());
			REQUIRE(aps.apsNormal().uncapped == normal.uncappedGaugeValue());
		}
	}
}

TEST_CASE("Skin loader leaves song jackets on native asynchronous asset path", "[Migration220][Skin]")
{
	struct Reset
	{
		~Reset() { noco::Asset::SetTextureLoader({}); }
	} reset;
	noco::Asset::SetTextureLoader([](FilePathView, noco::MipmapEnabledYN) { return Texture{}; },
		[](FilePathView path) { return String{ path }.replaced(U'\\', U'/').starts_with(U"imgs/"); });
	REQUIRE(noco::Asset::UsesCustomTextureLoader(U"imgs/bg.png"));
	REQUIRE(noco::Asset::UsesCustomTextureLoader(U"imgs\\bg.png"));
	REQUIRE_FALSE(noco::Asset::UsesCustomTextureLoader(U"songs/日本語/cover.png"));
	REQUIRE_FALSE(noco::Asset::UsesCustomTextureLoader(U"F:/songs/cover.png"));
	// Existing one-argument integrations keep their previous all-path behavior.
	noco::Asset::SetTextureLoader([](FilePathView, noco::MipmapEnabledYN) { return Texture{}; });
	REQUIRE(noco::Asset::UsesCustomTextureLoader(U"songs/cover.png"));
	noco::Asset::SetTextureLoader({});
	REQUIRE_FALSE(noco::Asset::UsesCustomTextureLoader(U"imgs/bg.png"));
}

TEST_CASE("Incremental 2.2 cache retains Unicode metadata and EX maximum across refreshes", "[Migration220][Select][Integration]")
{
	const FilePath root = FileSystem::PathAppend(FileSystem::CurrentDirectory(), U"migration220-fixture-" + Format(Time::GetMicrosec()));
	REQUIRE_FALSE(FileSystem::Exists(root));
	struct Cleanup
	{
		FilePath path;
		~Cleanup()
		{
			SelectMenuCacheDb::Invalidate(path);
			FileSystem::Remove(path);
		}
	} cleanup{ root };
	const FilePath song = FileSystem::PathAppend(root, U"おふとんクイズ会 2 Memorial Festival 中文 한국어 🎵");
	REQUIRE(FileSystem::CreateDirectories(song));
	const FilePath chart = FileSystem::PathAppend(song, U"譜面.ksh");
	const auto writeChart = [&](bool secondNote)
	{
		TextWriter writer{ chart };
		REQUIRE(writer.isOpen());
		writer.write(U"title=日本語 中文 한국어 🎵\nartist=fixture\ndifficulty=light\nlevel=1\nt=120\nver=171\n--\n0000|00|--\n1000|00|--\n");
		if (secondNote) writer.write(U"0100|00|--\n");
		writer.write(U"0000|00|--\n--\n");
	};
	const auto load = [&]
	{
		SelectMenuCacheDb::Loader loader{ Array<FilePath>{ root }, true };
		int remaining = 1000;
		while (!loader.isFinished() && --remaining > 0) loader.advance(Duration{ 0.002 });
		REQUIRE(loader.isFinished());
		REQUIRE(loader.progress().has_value());
		REQUIRE(loader.progress()->loadedChartCount == loader.progress()->totalChartCount);
		return SelectMenuCacheDb::LoadDirectoryForLevelSort(root, false, false);
	};
	writeChart(false);
	auto rows = load();
	REQUIRE(rows.size() == 1);
	REQUIRE(rows[0].chartInfo->title() == U"日本語 中文 한국어 🎵");
	REQUIRE(rows[0].chartInfo->exMaximum() == 5);
	writeChart(true);
	const auto cached = SelectMenuCacheDb::LoadDirectoryForLevelSort(root, false, false);
	REQUIRE(cached.size() == 1);
	REQUIRE(cached[0].chartInfo->exMaximum() == 5); // No unnecessary stat/parse on re-entry.
	rows = load();
	REQUIRE(rows.size() == 1);
	REQUIRE(rows[0].chartInfo->exMaximum() == 10);
	REQUIRE(rows[0].chartInfo->parentPath() == FileSystem::ParentPath(chart));
	// A damaged sibling must not hide the valid song or invalidate the whole folder.
	TextWriter{ FileSystem::PathAppend(song, U"broken.kson") }.write(U"{invalid-json");
	rows = load();
	REQUIRE(rows.size() == 1);
	REQUIRE(rows[0].chartInfo->exMaximum() == 10);
	SelectMenuCacheDb::Invalidate(root);
	rows = load();
	REQUIRE(rows.size() == 1);
	REQUIRE(rows[0].chartInfo->exMaximum() == 10);
}
