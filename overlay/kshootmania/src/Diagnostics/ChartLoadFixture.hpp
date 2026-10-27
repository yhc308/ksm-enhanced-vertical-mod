#pragma once
#include "Common/FsUtils.hpp"
#include "Scenes/Select/SelectChartInfo.hpp"
#include "Scenes/Select/SelectMenuCacheDb.hpp"
#include "MusicGame/Judgment/ExScore.hpp"
#include "MusicGame/Judgment/ButtonLaneJudgment.hpp"
#include "MusicGame/Judgment/LaserLaneJudgment.hpp"
#include "kson/IO/KshIO.hpp"
namespace ChartLoadFixture
{
	inline void Run()
	{
		if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"chart-load-fixture")))
			throw Error{ U"Chart loading requires isolated runtime" };
		TextWriter report{ FsUtils::GetResourcePath(U"chart-load-results.txt") };
		const auto step = [&](StringView label, const auto& action)
		{
			try
			{
				action();
				report.writeln(U"PASS " + String{ label });
			}
			catch (const std::exception& error)
			{
				report.writeln(U"FAIL {}: {}"_fmt(label, Unicode::FromUTF8(error.what())));
			}
			catch (const Error& error)
			{
				report.writeln(U"FAIL {}: {}"_fmt(label, error.what()));
			}
		};
		for (const auto& folder : FileSystem::DirectoryContents(FsUtils::GetResourcePath(U"songs"), Recursive::No))
		{
			if (!FileSystem::IsDirectory(folder))
				continue;
			for (const auto& file : FileSystem::DirectoryContents(folder, Recursive::Yes))
			{
				if (!FsUtils::HasChartExtension(file))
					continue;
				report.writeln(U"CHART " + file);
				step(U"metadata", [&]
					 {
						 const auto meta = SelectChartInfo::LoadMetaChartData(file);
						 report.writeln(U"meta_error={} title={}"_fmt(static_cast<int>(meta.error), Unicode::FromUTF8(meta.meta.title)));
					 });
				const auto chart = kson::LoadKshChartData(file.toUTF8());
				report.writeln(U"full_error={}"_fmt(static_cast<int>(chart.error)));
				const auto timing = kson::CreateTimingCache(chart.beat);
				for (size_t lane = 0; lane < chart.note.bt.size(); ++lane)
					step(U"BT {}"_fmt(lane), [&]
						 {
							 MusicGame::Judgment::ButtonLaneJudgment judgment{ JudgmentPlayMode::kOn, GaugeType::kNormalGauge, FastSlowMode::kHide, static_cast<Button>(kButtonBT_A + lane), chart.note.bt[lane], chart.beat, timing };
						 });
				for (size_t lane = 0; lane < chart.note.fx.size(); ++lane)
					step(U"FX {}"_fmt(lane), [&]
						 {
							 MusicGame::Judgment::ButtonLaneJudgment judgment{ JudgmentPlayMode::kOn, GaugeType::kNormalGauge, FastSlowMode::kHide, static_cast<Button>(kButtonFX_L + lane), chart.note.fx[lane], chart.beat, timing };
						 });
				for (size_t lane = 0; lane < chart.note.laser.size(); ++lane)
					step(U"laser {}"_fmt(lane), [&]
						 {
							 MusicGame::Judgment::LaserLaneJudgment judgment{ JudgmentPlayMode::kOn, static_cast<int32>(lane), chart.note.laser[lane], chart.beat, timing };
						 });
				step(U"EX maximum", [&]
					 {
						 report.writeln(U"ex={}"_fmt(MusicGame::Judgment::LoadChartExMaximum(file)));
					 });
				step(U"select chart info", [&]
					 {
						 const SelectChartInfo info{ file };
						 report.writeln(U"info_ex={}"_fmt(info.exMaximum()));
					 });
			}
			for (const bool rebuild : { true, false })
			{
				step(U"name cache rebuild={}"_fmt(rebuild), [&]
					 {
						 report.writeln(U"name_rows={}"_fmt(SelectMenuCacheDb::LoadDirectoryForNameSort(folder, rebuild).size()));
					 });
				step(U"level cache rebuild={}"_fmt(rebuild), [&]
					 {
						 report.writeln(U"level_rows={}"_fmt(SelectMenuCacheDb::LoadDirectoryForLevelSort(folder, rebuild).size()));
					 });
			}
		}
		report.writeln(U"COMPLETE");
	}
}