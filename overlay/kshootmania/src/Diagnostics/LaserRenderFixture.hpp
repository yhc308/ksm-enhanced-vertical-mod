#pragma once
#include "Diagnostics/DiagnosticArguments.hpp"
#include "MusicGame/Graphics/Highway/Note/LaserNoteGraphics.hpp"
#include "MusicGame/Scroll/HighwayScroll.hpp"
#include "Common/SkinPreset.hpp"
#include "Common/FsUtils.hpp"
#include <chrono>

namespace LaserRenderFixture
{
	inline void VerifyRenderContexts()
	{
		if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"laser-render-fixture")))
			throw Error{ U"Context tests require an isolated runtime" };
		const auto originalOverride = GameRender::Override();
		const bool originalEnabled = ConfigIni::GetBool(U"vertical_enabled", false);
		int checks = 0;
		const auto check = [&](bool value)
		{
			if (!value)
				throw Error{ U"Render context regression" };
			++checks;
		};
		GameRender::Override().reset();
		for (const bool enabled : { false, true })
		{
			ConfigIni::SetBool(U"vertical_enabled", enabled);
			check(GameRender::Current().outputSize == Scene::Size());
			check(GameRender::Current().vertical == enabled);
			{
				const GameRender::Scope outer{ { { 720, 1280 }, true } };
				check(GameRender::Size() == Size{ 720, 1280 } && GameRender::VerticalEnabled());
				{
					const GameRender::Scope inner{ { { 1280, 720 }, false } };
					check(GameRender::Size() == Size{ 1280, 720 } && !GameRender::VerticalEnabled());
				}
				check(GameRender::Size() == Size{ 720, 1280 } && GameRender::VerticalEnabled());
			}
			check(!GameRender::Override() && GameRender::VerticalEnabled() == enabled);
		}
		ConfigIni::SetBool(U"vertical_enabled", originalEnabled);
		GameRender::Override() = originalOverride;
		TextWriter{ FsUtils::GetResourcePath(U"render-context-passed.txt") }.writeln(U"{} fallback, explicit context and nested restoration checks passed"_fmt(checks));
	}

	inline bool LegacyDrawRange(const MusicGame::Scroll::HighwayScrollContext& context, kson::Pulse start, kson::Pulse end, int32 offset, const MusicGame::Scroll::HighwayScroll& scroll, const kson::BeatInfo& beat, const kson::TimingCache& timing, const MusicGame::GameStatus& status)
	{
		if (end < start)
			std::swap(start, end);
		const auto position = [&](kson::Pulse pulse)
		{
			return scroll.getPositionY(pulse, beat, timing, status);
		};
		int32 minimum = position(start) + offset, maximum = minimum;
		const auto add = [&](kson::Pulse pulse)
		{
			const int32 value = position(pulse) + offset;
			minimum = Min(minimum, value);
			maximum = Max(maximum, value);
		};
		if (context.hasNegativeScrollSpeed())
			for (auto pulse = start + kson::kResolution4 / 16; pulse < end; pulse += kson::kResolution4 / 16)
				add(pulse);
		add(end);
		return maximum >= 0 && minimum < MusicGame::Graphics::kHighwayTextureSize.y;
	}

	inline void VerifyRanges()
	{
		using namespace MusicGame;
		if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"laser-render-fixture")))
			throw Error{ U"Range tests require an isolated runtime" };
		TextWriter report{ FsUtils::GetResourcePath(U"laser-range-results.csv") };
		report.writeln(U"chart,checks,reverse_scroll_checks,status");
		int64 total = 0;
		for (const auto& file : FileSystem::DirectoryContents(FsUtils::GetResourcePath(U"laser-fixture-charts"), Recursive::No))
		{
			if (!FsUtils::HasChartExtension(file))
				continue;
			auto chart = FsUtils::HasKsonExtension(file) ? kson::LoadKsonChartData(file.toUTF8()) : kson::LoadKshChartData(file.toUTF8());
			if (chart.error != kson::ErrorType::None)
				throw Error{ U"Range chart failed" };
			for (auto& lane : chart.note.laser)
				for (auto& [pulse, section] : lane)
					section = kson::ExpandCurveSegments(section, kson::kCurveSubdivisionInterval);
			chart.beat.scrollSpeed = kson::BakeStopIntoScrollSpeed(kson::ExpandCurveSegments(chart.beat.scrollSpeed, kson::kCurveSubdivisionInterval), chart.beat.stop);
			const auto timing = kson::CreateTimingCache(chart.beat);
			Scroll::HighwayScroll scroll{ chart };
			Array<std::pair<kson::Pulse, kson::Pulse>> ranges;
			for (const auto& lane : chart.note.laser)
				for (const auto& [pulse, section] : lane)
				{
					ranges.emplace_back(pulse, pulse + section.v.rbegin()->first);
					int point = 0;
					for (auto it = section.v.begin(); std::next(it) != section.v.end(); ++it)
						if (point++ % 16 == 0)
							ranges.emplace_back(pulse + it->first, pulse + std::next(it)->first);
				}
			const auto addNotes = [&](const auto& lanes)
			{
				for (const auto& lane : lanes)
					for (const auto& [pulse, note] : lane)
						if (note.length > 0)
							ranges.emplace_back(pulse, pulse + note.length);
			};
			addNotes(chart.note.bt);
			addNotes(chart.note.fx);
			const auto last = ranges.empty() ? 20000 : std::max_element(ranges.begin(), ranges.end(), [](const auto& a, const auto& b)
																		{
																			return a.second < b.second;
																		})
														   ->second;
			// Include nonregistered, reversed, empty and sample-boundary ranges.
			for (int i = 0; i < 64; ++i)
				ranges.emplace_back((i * 7919) % last, (i * 7919) % last + std::array<kson::Pulse, 8>{ 0, 1, 59, 60, 61, 120, 121, 960 }[i % 8]);
			int64 checks = 0, cached = 0;
			for (const HispeedSetting setting : { HispeedSetting{ HispeedType::XMod, 10 }, { HispeedType::XMod, 30 }, { HispeedType::OMod, 100 }, { HispeedType::OMod, 450 }, { HispeedType::OMod, 850 }, { HispeedType::CMod, 450 } })
				for (int frame = 0; frame < 16; ++frame)
				{
					GameStatus status;
					// Alternating direction covers rewinds and large forward seeks.
					status.currentPulseDouble = static_cast<double>(last) * ((frame * 7) % 16) / 15.0 + .375;
					status.currentPulse = static_cast<kson::Pulse>(status.currentPulseDouble);
					status.currentTimeSec = kson::PulseDoubleToSec(status.currentPulseDouble, chart.beat, timing);
					scroll.update(setting, kson::TempoAt(status.currentPulse, chart.beat));
					const Scroll::HighwayScrollContext context{ &scroll, &chart.beat, &timing, &status };
					for (const auto& [start, end] : ranges)
						for (const int32 offset : { -6, 0, 17 })
						{
							const bool reverse = (checks % 2) != 0;
							const auto a = reverse ? end : start, b = reverse ? start : end;
							if (context.isPulseRangeInDrawRange(a, b, offset) != LegacyDrawRange(context, a, b, offset, scroll, chart.beat, timing, status))
								throw Error{ U"Range mismatch: {} {} {} {}"_fmt(file, a, b, status.currentPulseDouble) };
							++checks;
							cached += context.hasNegativeScrollSpeed();
						}
				}
			total += checks;
			report.writeln(U"{},{},{},PASS"_fmt(FileSystem::BaseName(file), checks, cached));
		}
		TextWriter{ FsUtils::GetResourcePath(U"laser-range-passed.txt") }.writeln(U"{} legacy visibility comparisons passed"_fmt(total));
	}

	inline void Run()
	{
		using namespace MusicGame;
		using namespace MusicGame::Graphics;
		if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"laser-render-fixture")))
			throw Error{ U"Laser render tests require an isolated runtime" };
		const auto clock = []
		{
			return std::chrono::steady_clock::now();
		};
		const auto ms = [](auto start)
		{
			return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		};
		TextWriter report{ FsUtils::GetResourcePath(U"laser-render.csv") };
		report.writeln(U"chart,vertical,width,repeat,time_sec,draw_ms,flush_ms,frame_ms");
		const auto originalSkin = SkinPreset::Current();
		for (const auto& file : FileSystem::DirectoryContents(FsUtils::GetResourcePath(U"laser-fixture-charts"), Recursive::No))
		{
			if (!FsUtils::HasChartExtension(file))
				continue;
			auto chart = FsUtils::HasKsonExtension(file) ? kson::LoadKsonChartData(file.toUTF8()) : kson::LoadKshChartData(file.toUTF8());
			if (chart.error != kson::ErrorType::None)
				throw Error{ U"Fixture chart failed: " + file };
			std::array<HashSet<kson::Pulse>, kson::kNumLaserLanesSZ> curves;
			kson::Pulse lastPulse = 0;
			for (size_t lane = 0; lane < curves.size(); ++lane)
				for (auto& [pulse, section] : chart.note.laser[lane])
				{
					for (const auto& [relative, point] : section.v)
						if (!point.curve.isLinear())
							curves[lane].insert(pulse + relative);
					section = kson::ExpandCurveSegments(section, kson::kCurveSubdivisionInterval);
					lastPulse = Max(lastPulse, pulse + section.v.rbegin()->first);
				}
			chart.beat.scrollSpeed = kson::ExpandCurveSegments(chart.beat.scrollSpeed, kson::kCurveSubdivisionInterval);
			chart.beat.scrollSpeed = kson::BakeStopIntoScrollSpeed(chart.beat.scrollSpeed, chart.beat.stop);
			const auto timing = kson::CreateTimingCache(chart.beat);
			const double duration = Max(10.0, kson::PulseToSec(lastPulse, chart.beat, timing));
			const String name = FileSystem::BaseName(file);
			for (const bool vertical : { false, true })
				for (const double width : { 50.0, SkinPreset::kVerticalLaserPercent, 100.0, 125.0 })
				{
					const GameRender::Scope scope{ GameRender::Context{ vertical ? Size{ 720, 1280 } : Size{ 1280, 720 }, vertical } };
					auto skin = originalSkin;
					skin.laserWidthPercent = width;
					SkinPreset::Activate(skin);
					Scroll::HighwayScroll scroll{ chart };
					LaserNoteGraphics laser;
					HighwayRenderTexture target;
					PlayOption option;
					for (int repeat = 0; repeat < 3; ++repeat)
						for (int sample = -2; sample < 48; ++sample)
						{
							const double time = duration * Max(sample, 0) / 47;
							GameStatus status;
							status.currentTimeSec = time;
							status.currentPulseDouble = kson::SecToPulseDouble(time, chart.beat, timing);
							status.currentPulse = static_cast<kson::Pulse>(status.currentPulseDouble);
							scroll.update({ HispeedType::OMod, 450 }, kson::TempoAt(status.currentPulse, chart.beat));
							const auto frameStart = clock();
							if (!System::Update())
								throw Error{ U"Laser fixture interrupted" };
							target.additiveTexture().clear(Palette::Black);
							target.invMultiplyTexture().clear(Palette::Black);
							const Scroll::HighwayScrollContext context{ &scroll, &chart.beat, &timing, &status };
							const auto drawStart = clock();
							laser.draw(chart, curves, option, status, context, target);
							const double drawMs = ms(drawStart);
							const auto flushStart = clock();
							Graphics2D::Flush();
							const double flushMs = ms(flushStart);
							target.additiveTexture().scaled(.5).drawAt(Scene::Center());
							const double frameMs = ms(frameStart);
							if (sample >= 0)
								report.writeln(U"{},{},{},{},{},{},{},{}"_fmt(name, vertical, width, repeat, time, drawMs, flushMs, frameMs));
							if (repeat == 0 && sample >= 0 && sample % 8 == 0)
							{
								Image image;
								target.additiveTexture().readAsImage(image);
								image.save(FsUtils::GetResourcePath(U"laser-{}-{}-{}-{}-main.png"_fmt(name, vertical, static_cast<int>(width * 100), sample)));
								target.invMultiplyTexture().readAsImage(image);
								image.save(FsUtils::GetResourcePath(U"laser-{}-{}-{}-{}-mask.png"_fmt(name, vertical, static_cast<int>(width * 100), sample)));
							}
						}
				}
		}
		SkinPreset::Activate(originalSkin);
		TextWriter{ FsUtils::GetResourcePath(U"laser-render-passed.txt") }.writeln(U"Completed real laser draws in both modes, four widths, three repetitions");
	}
}