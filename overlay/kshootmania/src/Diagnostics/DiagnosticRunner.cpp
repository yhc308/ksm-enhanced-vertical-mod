#include "DiagnosticRunner.hpp"
#include "DiagnosticArguments.hpp"
#if KSM_ENABLE_DIAGNOSTICS
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
		if (ModDiagnostics::Arguments().contains(U"--ex-score-test"))
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
		if (ModDiagnostics::Arguments().contains(U"--timing-hud-smoke-test"))
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
		if (ModDiagnostics::Arguments().contains(U"--audio-timing-smoke-test"))
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
		if (ModDiagnostics::Arguments().contains(U"--audio-calibration-smoke-test"))
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
		if (SkinFixture::Enabled() && ModDiagnostics::Arguments().contains(U"--skin-core-test"))
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
		if (ModDiagnostics::Arguments().contains(U"--judge-setting-test"))
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
		if (ModDiagnostics::Arguments().contains(U"--aps-video-fixture"))
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
		if (ModDiagnostics::Arguments().contains(U"--aps-test"))
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
		if (ModDiagnostics::Arguments().contains(U"--audio-end-game-test"))
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

#else
namespace DiagnosticRunner
{
	bool RunAfterAudioInitialization()
	{
		return false;
	}
	bool RunAfterAssetRegistration()
	{
		return false;
	}
	bool RunAfterInputInitialization()
	{
		return false;
	}
}
#endif
