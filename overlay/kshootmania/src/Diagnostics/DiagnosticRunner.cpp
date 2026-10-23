#include "DiagnosticRunner.hpp"
#include "AudioTimingFixture.hpp"
#include "AudioEndGameFixture.hpp"
#include "ApsVideoFixture.hpp"
#include "TimingHudFixture.hpp"
#include "ExScoreFixture.hpp"
#include "ApsFixture.hpp"
#include "JudgeSettingFixture.hpp"
#include "RenderGeometryFixture.hpp"
#include "SkinPresetFixture.hpp"
#include "Ini/AudioCalibration.hpp"
#include "Vertical/OptionsPresentation.hpp"
#include "ksmaudio/ksmaudio.hpp"

namespace DiagnosticRunner
{
	bool RunAfterAudioInitialization()
	{
		if (System::GetCommandLineArgs().contains(U"--ex-score-test"))
		{
			try
			{
				ExScoreFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FsUtils::GetResourcePath(U"ex-score-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (System::GetCommandLineArgs().contains(U"--timing-hud-smoke-test"))
		{
			try
			{
				TimingHudFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FsUtils::GetResourcePath(U"timing-hud-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (System::GetCommandLineArgs().contains(U"--audio-timing-smoke-test"))
		{
			try
			{
				AudioTimingFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"audio-timing-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (System::GetCommandLineArgs().contains(U"--audio-calibration-smoke-test"))
		{
			AudioCalibration::RunFixtureTests();
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		return false;
	}

	bool RunAfterAssetRegistration()
	{
		if (RenderGeometryFixture::Requested())
		{
			RenderGeometryFixture::Run();
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (SkinFixture::Enabled() && System::GetCommandLineArgs().contains(U"--skin-core-test"))
		{
			SkinFixture::Core();
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		return false;
	}

	bool RunAfterInputInitialization()
	{
		if (System::GetCommandLineArgs().contains(U"--judge-setting-test"))
		{
			try
			{
				JudgeSettingFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FsUtils::GetResourcePath(U"judge-setting-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (System::GetCommandLineArgs().contains(U"--aps-video-fixture"))
		{
			try
			{
				ApsVideoFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FsUtils::GetResourcePath(U"aps-video-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (System::GetCommandLineArgs().contains(U"--aps-test"))
		{
			try
			{
				ApsFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FsUtils::GetResourcePath(U"aps-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		if (System::GetCommandLineArgs().contains(U"--audio-end-game-test"))
		{
			try
			{
				AudioEndGameFixture::Run();
			}
			catch (const Error& error)
			{
				TextWriter{ FsUtils::GetResourcePath(U"audio-end-game-failure.txt") }.writeln(error);
			}
			Vertical::OptionsPresentation::Reset();
			ksmaudio::Terminate();
			return true;
		}
		return false;
	}
}
