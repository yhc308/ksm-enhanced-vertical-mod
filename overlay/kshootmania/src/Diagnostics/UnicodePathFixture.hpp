#pragma once
#include "Common/CommandLine.hpp"
#include "Common/AssetManagement.hpp"
#include "Common/Encoding.hpp"
#include "Common/CsvUtils.hpp"
#include "Common/FsUtils.hpp"
#include "Scenes/Select/SelectMenuCacheDb.hpp"
#include "HighScore/KscIO.hpp"
#include "HighScore/AdditionalScoreIO.hpp"
#include "MusicGame/PlayResult.hpp"
#include "ksmaudio/Stream.hpp"
#include "ksmaudio/Sample.hpp"
#include <SQLiteCpp/SQLiteCpp.h>

namespace UnicodePathFixture
{
	inline void Run()
	{
		if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"unicode-path-fixture")))
			throw Error{ U"Unicode path tests require an isolated runtime" };
		TextWriter report{ FsUtils::GetResourcePath(U"unicode-path-results.txt") };
		int checks = 0, failures = 0;
		const auto check = [&](bool ok, StringView label)
		{
			++checks;
			if (!ok)
				++failures;
			report.writeln(U"{} {}"_fmt(ok ? U"PASS" : U"FAIL", label));
		};
		check(CsvUtils::ParseRow(U"\"目录,フォルダ\",\"表示,🎵\"") == Optional<Array<String>>{ Array<String>{ U"目录,フォルダ", U"表示,🎵" } }, U"quoted CSV commas");
		check(CsvUtils::ParseRow(U"a,\"b\"\"c\"") == Optional<Array<String>>{ Array<String>{ U"a", U"b\"c" } }, U"CSV escaped quote");
		check(!CsvUtils::ParseRow(U"a,\"unfinished"), U"unfinished CSV quote rejected");
		check(CsvUtils::ParseRow(U"\" a \",") == Optional<Array<String>>{ Array<String>{ U" a ", U"" } }, U"quoted folder whitespace and empty display preserved");
		check(MusicGame::Judgment::LoadChartExMaximum(FsUtils::GetResourcePath(U"invalid-bpm.kson")) == 0, U"invalid BPM has no fabricated EX maximum");
		for (const String name : { U"cp932", U"utf8-bom", U"utf8-ascii" })
		{
			const SelectChartInfo info{ FsUtils::GetResourcePath(name + U".ksh") };
			check(!info.hasError() && info.title() == (name == U"utf8-ascii" ? U"ASCII chart" : U"日本語 譜面"), U"legacy encoding " + name);
			check(info.exMaximum() == 5, U"implicit 4/4 EX compatibility " + name);
		}
		const JSON cases = JSON::Load(FsUtils::GetResourcePath(U"unicode-cases.json"));
		check(!cases.isEmpty(), U"fixture cases loaded");
		const auto& args = CommandLine::Arguments();
		check(args.includes(U"会 简体 繁體 한글 𠮷 🎵 👩🏽‍💻"), U"Unicode command line is lossless");
		for (const auto& item : cases.arrayView())
		{
			const FilePath chart = FsUtils::GetResourcePath(item[U"path"].getString());
			const String expectedTitle = item[U"title"].getString();
			try
			{
				const SelectChartInfo info{ chart };
				check(!info.hasError() && info.title() == expectedTitle, chart + U" metadata");
				check(info.exMaximum() == item[U"maximum"].get<int64>(), chart + U" EX maximum unchanged");
				const Image jacket{ info.jacketFilePath() };
				check(!jacket.isEmpty(), chart + U" jacket");
				for (const bool preload : { false, true })
				{
					ksmaudio::Stream stream{ FileSystem::PathAppend(FileSystem::ParentPath(chart), U"音楽 简体 繁體 한글 🎵.wav").toUTF8(), 0, false, preload };
					check(stream.duration().count() > 0, chart + U" audio preload=" + Format(preload));
				}
				// Verify BASS uses the same wide-path overload as Sample::LoadSample.
				ksmaudio::Sample sample{ FileSystem::PathAppend(FileSystem::ParentPath(chart), U"音楽 简体 繁體 한글 🎵.wav").toUTF8() };
				const auto samplePath = FileSystem::PathAppend(FileSystem::ParentPath(chart), U"音楽 简体 繁體 한글 🎵.wav").toWstr();
				const HSAMPLE handle = BASS_SampleLoad(FALSE, samplePath.c_str(), 0, 0, 1, BASS_SAMPLE_OVER_POS);
				check(handle != 0, chart + U" sample file load");
				if (handle)
					BASS_SampleFree(handle);
				const FilePath folder = FileSystem::ParentPath(FileSystem::ParentPath(chart));
				for (const bool rebuild : { true, false })
				{
					const auto songs = SelectMenuCacheDb::LoadDirectoryForNameSort(folder, rebuild);
					const auto levels = SelectMenuCacheDb::LoadDirectoryForLevelSort(folder, rebuild);
					check(songs.size() == 1 && levels.size() == 1 && levels.front().chartInfo->title() == expectedTitle,
						  chart + U" cold/warm name and level index " + Format(rebuild));
				}
				MusicGame::PlayResult result;
				result.score = 9000000;
				result.exScore = 7000;
				result.exMaximum = 8013;
				result.totalCombo = 10;
				result.comboStats.critical = 10;
				check(KscIO::WriteHighScoreInfo(chart, result, KscKey{}), chart + U" score write");
				check(KscIO::ReadHighScoreInfo(chart, KscKey{}).exScore(GaugeType::kNormalGauge) == Optional<int64>{ 7000 }, chart + U" EX record reread");
			}
			catch (const Error& error)
			{
				check(false, chart + U": " + error.what());
			}
			catch (const std::exception& error)
			{
				check(false, chart + U": " + Unicode::FromUTF8(error.what()));
			}
		}
		// Corrupt cached values must never become array indexes or paths.
		const FilePath cacheRoot = FsUtils::GetResourcePath(U"cache/songs_index/v4");
		for (const auto& path : FileSystem::DirectoryContents(cacheRoot, Recursive::Yes))
		{
			if (FileSystem::Extension(path) != U"sqlite3")
				continue;
			FilePath source;
			{
				SQLite::Database db(path.toUTF8(), SQLite::OPEN_READWRITE);
				SQLite::Statement query(db, "SELECT value FROM cache_meta WHERE key='directory_path'");
				if (!query.executeStep())
					continue;
				source = Unicode::FromUTF8(query.getColumn(0).getString());
				db.exec("UPDATE charts SET difficulty_idx=99");
			}
			check(SelectMenuCacheDb::LoadDirectoryForLevelSort(source, false).size() == 1, U"invalid cache difficulty recovers in memory");
			{
				SQLite::Database db(path.toUTF8(), SQLite::OPEN_READWRITE);
				db.exec("UPDATE charts SET difficulty_idx=3, chart_path='../outside.ksh'");
			}
			const auto recovered = SelectMenuCacheDb::LoadDirectoryForLevelSort(source, false);
			check(recovered.size() == 1 && recovered.front().chartInfo->title() != U"", U"cache path traversal rejected and original chart retained");
			{
				BinaryWriter writer{ path };
				writer.write("not a database", 14);
			}
			check(SelectMenuCacheDb::LoadDirectoryForLevelSort(source, false).size() == 1, U"corrupt SQLite recovers in memory");
			check(FileSystem::Size(path) == 14, U"corrupt cache preserved for diagnosis");
		}
		for (const String language : { U"English", U"Japanese", U"Korean", U"Simplified Chinese", U"Traditional Chinese" })
		{
			I18n::LoadLanguage(language);
			AssetManagement::RefreshSystemFontForCurrentLanguage();
			const Font font = AssetManagement::SystemFont();
			check(static_cast<bool>(font), language + U" system font");
			RenderTexture target{ Size{ 1200, 70 }, ColorF{ 0.05 } };
			{
				const ScopedRenderTarget2D renderTarget{ target };
				font(U"会 简体中文 繁體中文 한글 𠮷 🎵 👩🏽‍💻").draw(30, 10, 10);
			}
			Graphics2D::Flush();
			Image image;
			target.readAsImage(image);
			check(image.save(FsUtils::GetResourcePath(U"font-" + language + U".png")), language + U" mixed script render");
		}
		report.writeln(U"TOTAL checks={} failures={}"_fmt(checks, failures));
	}
}
