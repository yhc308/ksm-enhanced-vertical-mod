#include <catch2/catch.hpp>
#include "Common/SkinPreset.hpp"
#include "Ini/ConfigIni.hpp"
#include "RuntimeConfig.hpp"
#include "MusicGame/TimingFeedback.hpp"
#include "Graphics/GameRenderContext.hpp"
#include "HighScore/AdditionalScoreIO.hpp"
#include "Common/FsUtils.hpp"
#include <filesystem>

TEST_CASE("Timing text holds 850ms and fades over 250ms in both orientations", "[Timing][Patch]")
{
	using namespace MusicGame;
	for (const bool vertical : { false, true })
	{
		const GameRender::Scope scope{ { vertical ? Size{ 720, 1280 } : Size{ 1280, 720 }, vertical } };
		TimingHudSettings settings;
		settings.display = TimingMsDisplay::kAll;
		const TimingFeedback event{ Judgment::JudgmentResult::kNearFast, 75, 0 };
		REQUIRE(TimingOpacity(-.001) == 0);
		for (const double hold : { 0.0, .849, .850 })
			REQUIRE(TimingOpacity(hold) == Approx(.9));
		REQUIRE(TimingOpacity(.975) == Approx(.45));
		REQUIRE(TimingOpacity(1.099) > 0);
		REQUIRE(TimingOpacity(1.100) == 0);
		REQUIRE(PresentTiming(settings, event, 1.099).ms != U"");
		REQUIRE(PresentTiming(settings, event, 1.100).ms.isEmpty());
		REQUIRE(PresentTiming(settings, event, 1.100).word.isEmpty());
		REQUIRE(PresentTiming(settings, event, .975).alpha == Approx(.45));
		REQUIRE(PresentTiming(settings, event, .975).wordAlpha == Approx(.45));
		const TimingFeedback next{ Judgment::JudgmentResult::kNearSlow, -75, 1.0 };
		REQUIRE(PresentTiming(settings, next, 1.0).alpha == Approx(.9));
		REQUIRE(PresentTiming(settings, next, 1.0).wordAlpha == Approx(.9));
	}
}

TEST_CASE("Viewport presentation settings preserve defaults and migrate old OFF", "[Skin][Patch]")
{
	using namespace SkinPreset;
	Settings settings;
	settings.sanitize();
	REQUIRE(settings.slamThicknessPercent == 100);
	REQUIRE(settings.keyBeamLengthPercent == 100);
	REQUIRE(settings.keyBeamOpacityPercent == 100);
	for (int mode = 1; mode <= 5; ++mode)
	{
		settings.keyBeamMode = static_cast<KeyBeamMode>(mode);
		settings.sanitize();
		REQUIRE(static_cast<int>(settings.keyBeamMode) == mode);
	}
	settings.keyBeamMode = KeyBeamMode::Off;
	settings.sanitize();
	REQUIRE(settings.keyBeamMode == KeyBeamMode::Normal);
	REQUIRE(settings.keyBeamOpacityPercent == 0);
	for (int length : { 25, 50, 75, 100 })
		for (int alpha = 0; alpha <= 100; alpha += 10)
		{
			settings.keyBeamLengthPercent = length;
			settings.keyBeamOpacityPercent = alpha;
			settings.sanitize();
			REQUIRE(settings.keyBeamLengthPercent == length);
			REQUIRE(settings.keyBeamOpacityPercent == alpha);
		}
	settings.slamThicknessPercent = 500;
	settings.laserWidthPercent = 200;
	settings.sanitize();
	REQUIRE(settings.slamThicknessPercent == 200);
	REQUIRE(settings.laserWidthPercent == 200);
	REQUIRE(StepLaserPercent(200, 1) == 200);
	REQUIRE(StepLaserPercent(50, -1) == 50);
	REQUIRE(StepLaserPercent(70, 1) == Approx(kVerticalLaserPercent));
	REQUIRE(StepLaserPercent(kVerticalLaserPercent, 1) == 75);
	REQUIRE(StepLaserPercent(kVerticalLaserPercent, -1) == 70);
}

TEST_CASE("EFF.RATE persistence is opt-in and preserves the selected APS mode", "[Gauge][Persistence][Patch]")
{
	struct Restore
	{
		bool enabled = ConfigIni::GetBool(ConfigIni::Key::kRememberEffRate, false);
		int saved = ConfigIni::GetInt(ConfigIni::Key::kLastEffRate, 1);
		int runtime = RuntimeConfig::GetGaugeSelection();
		~Restore()
		{
			RuntimeConfig::SetRememberEffRate(false);
			RuntimeConfig::SetGaugeSelection(runtime);
			ConfigIni::SetBool(ConfigIni::Key::kRememberEffRate, enabled);
			ConfigIni::SetInt(ConfigIni::Key::kLastEffRate, saved);
		}
	} restore;
	for (int selection = 0; selection <= 3; ++selection)
	{
		RuntimeConfig::SetRememberEffRate(false);
		RuntimeConfig::SetGaugeSelection(selection);
		RuntimeConfig::SetRememberEffRate(true);
		REQUIRE(ConfigIni::GetInt(ConfigIni::Key::kLastEffRate) == selection);
		RuntimeConfig::SetRememberEffRate(false);
		RuntimeConfig::SetGaugeSelection(1);
		ConfigIni::SetBool(ConfigIni::Key::kRememberEffRate, true);
		RuntimeConfig::RestoreGaugeSelectionFromConfigIni();
		REQUIRE(RuntimeConfig::GetGaugeSelection() == selection);
		REQUIRE(RuntimeConfig::GetApsEnabled() == (selection == 3));
	}
	RuntimeConfig::SetGaugeSelection(3);
	RuntimeConfig::SetRememberEffRate(false);
	REQUIRE(RuntimeConfig::GetGaugeSelection() == 3);
	RuntimeConfig::RestoreGaugeSelectionFromConfigIni();
	REQUIRE(RuntimeConfig::GetGaugeSelection() == 1);
	RuntimeConfig::SetRememberEffRate(true);
	for (int bad : { -1, 4, 9999 })
	{
		ConfigIni::SetInt(ConfigIni::Key::kLastEffRate, bad);
		RuntimeConfig::RestoreGaugeSelectionFromConfigIni();
		REQUIRE(RuntimeConfig::GetGaugeSelection() == 1);
	}
}

TEST_CASE("Additional MAX is orientation independent and legacy best EX survives", "[EX][Persistence][Patch]")
{
	const auto directory = std::filesystem::current_path() / ("patch-score-" + std::to_string(Time::GetMicrosec()));
	REQUIRE(std::filesystem::create_directory(directory));
	struct Cleanup
	{
		std::filesystem::path path;
		~Cleanup()
		{
			std::error_code error;
			for (const auto* name : { L"portrait.ksc", L"portrait_additional.ksc", L"landscape.ksc", L"landscape_additional.ksc" })
				std::filesystem::remove(path / name, error);
			std::filesystem::remove(path, error); // Refuses if any unexpected file remains.
		}
	} cleanup{ directory };
	const String key = U"normal,normal,x10,on,on,on";
	for (bool vertical : { false, true })
	{
		const GameRender::Scope scope{ { vertical ? Size{ 720, 1280 } : Size{ 1280, 720 }, vertical } };
		const FilePath original = Unicode::FromWstring((directory / (vertical ? L"portrait.ksc" : L"landscape.ksc")).wstring());
		const FilePath extra = FsUtils::EliminateExtension(original) + U"_additional.ksc";
		const String base = key + U"=9000000,0,10,0,10,10,0,0,0";
		{
			TextWriter writer{ original };
			writer.writeln(base);
		}
		{
			TextWriter writer{ extra };
			writer.writeln(base + U",95");
		}
		MusicGame::PlayResult result;
		result.exScore = 90;
		result.exMaximum = 100;
		REQUIRE(AdditionalScoreIO::Write(original, result, KscKey{}));
		const auto row = TextReader{ extra }.readAll().trimmed();
		const auto fields = row.substr(row.indexOf(U'=') + 1).split(U',');
		REQUIRE(fields.size() == 11);
		REQUIRE(fields[9] == U"95");
		REQUIRE(fields[10] == U"100");
		REQUIRE(TextReader{ original }.readAll().trimmed() == base);
		result.exMaximum = 0;
		REQUIRE_FALSE(AdditionalScoreIO::Write(original, result, KscKey{}));
		REQUIRE(TextReader{ extra }.readAll().trimmed() == row);
	}
}
