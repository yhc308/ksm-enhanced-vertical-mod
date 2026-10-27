#pragma once
#include "Common/ErrorReporting.hpp"
#include "Common/SongLoadDiagnostics.hpp"
#include "Common/AssetManagement.hpp"
#include "Common/FsUtils.hpp"
#include "Graphics/ScreenUtils.hpp"
#include "Graphics/GameRenderContext.hpp"
#include <NocoUI.hpp>
#include <filesystem>
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif

namespace ErrorReportFixture
{
	inline void RequireIsolated()
	{
		if (!FileSystem::IsFile(FsUtils::GetResourcePath(U"error-dialog-fixture")))
			throw Error{ U"Error reporting tests require an isolated runtime" };
	}

	inline void Run()
	{
		RequireIsolated();
		using namespace ErrorReporting;
		TextWriter results{ FsUtils::GetResourcePath(U"error-report-tests.txt") };
		int checks = 0;
		const auto check = [&](bool ok, StringView label)
		{
			if (!ok)
				throw Error{ U"Error report check failed: " + String{ label } };
			++checks;
			results.writeln(U"PASS " + String{ label });
		};
		const FilePath path = FsUtils::GetResourcePath(U"songs/おふとんクイズ会 简体 繁體 한글 👩🏽‍💻/IN.ksh");
		const auto lookup = Record(Stage::Metadata, path, std::out_of_range{ "invalid map<K, T> key" });
		check(lookup.reason == Reason::Lookup && Code(lookup.reason) == U"SEL-003", U"map lookup classified without guessing encoding");
		check(lookup.logSaved, U"log write verified");
		check(TextReader{ LogPath(lookup) }.readAll().includes(path), U"full Unicode path retained in log");
		check(TextReader{ LogPath(lookup) }.readAll().includes(U"invalid map<K, T> key"), U"original exception retained");
		check(TextReader{ LogPath(lookup) }.readAll().includes(lookup.id), U"window and log share event ID");
		const auto missing = FileUnavailable(Stage::CommandLine, path);
		check(missing.reason == Reason::MissingFile, U"actual missing file classified");
		const auto denied = Record(Stage::Chart, path, std::filesystem::filesystem_error{ "access", std::make_error_code(std::errc::permission_denied) });
		check(denied.reason == Reason::AccessDenied, U"access denied uses OS code");
		const auto longPath = Record(Stage::Chart, path, std::filesystem::filesystem_error{ "length", std::make_error_code(std::errc::filename_too_long) });
		check(longPath.reason == Reason::PathTooLong, U"path length uses OS code");
		const auto unknown = Record(Stage::Game, U"", U"unrecognized exception");
		check(unknown.reason == Reason::Unknown && LogPath(unknown).ends_with(U"log/error.log"), U"unknown global error has separate log");
		check(unknown.id != lookup.id, U"events have unique IDs");
		check(!FormatMessage(Record(Stage::Game, U"", U"\n")).isEmpty(), U"empty exception line is safe");
		check(ShortPath(U"C:/" + String(200, U'字') + U".ksh").size() < 80, U"very long filename does not overflow dialog");
		const String deep = U"C:/" + String(200, U'x') + U"/音楽 👩🏽‍💻/IN.ksh";
		check(ShortPath(deep).ends_with(U"音楽 👩🏽‍💻/IN.ksh"), U"short path preserves whole emoji sequence");
		{
			SongLoadDiagnostics::Batch batch;
			SongLoadDiagnostics::Write(U"chart metadata", path, U"bad chart");
			{
				SongLoadDiagnostics::Batch nested;
				SongLoadDiagnostics::Write(U"cache fallback", path, U"recovered");
				check(nested.reports.size() == 1, U"nested diagnostics isolated");
			}
			SongLoadDiagnostics::Write(U"select chart", path, U"bad sibling");
			check(batch.reports.size() == 2, U"diagnostics scope restored");
		}
#ifdef _WIN32
		const HANDLE lock = ::CreateFileW(LogPath(lookup).toWstr().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		check(lock != INVALID_HANDLE_VALUE, U"lock own test log");
		const auto blocked = Record(Stage::Chart, path, U"Cannot save this diagnostic");
		::CloseHandle(lock);
		check(!blocked.logSaved, U"failed log save reported truthfully");
#endif
		const Size originalSize = Scene::Size();
		for (const String language : { U"English", U"Japanese", U"Korean", U"Simplified Chinese", U"Traditional Chinese" })
		{
			I18n::LoadLanguage(language);
			noco::SetGlobalDefaultFont(AssetManagement::SystemFont());
			const String message = FormatMessage(lookup);
			check(message.includes(U"SEL-003") && message.includes(lookup.id) && message.includes(U"log/song-loading.log"), language + U" diagnostic identity");
			check(!message.includes(U"UTF-8") && !message.includes(U"Shift-JIS"), language + U" no speculative cause");
			Report unsaved = lookup;
			unsaved.logSaved = false;
			check(!FormatMessage(unsaved).includes(U"log/song-loading.log"), language + U" unsaved log not advertised");
			for (const Size size : { Size{ 640, 480 }, Size{ 1280, 720 }, Size{ 720, 1280 }, Size{ 1080, 1920 } })
			{
				Scene::Resize(size);
				const GameRender::Scope scope{ { size, size.y > size.x } };
				const auto canvas = noco::Canvas::LoadFromFile(FsUtils::GetResourcePath(U"mods/enhanced/ui/error_dialog.noco"));
				check(static_cast<bool>(canvas), U"error canvas loads");
				canvas->setParamValue(U"titleText", U"ERROR");
				canvas->setParamValue(U"bodyText", message);
				canvas->setParamValue(U"hasTitle", true);
				canvas->setAutoFitMode(noco::AutoFitMode::None);
				const double scale = Min(ScreenUtils::Scaled(1.0), Min(size.x * .9 / canvas->referenceSize().x, size.y * .9 / canvas->referenceSize().y));
				const Vec2 position = (Scene::Size() - canvas->referenceSize() * scale) / 2.0;
				check(position.x >= 0 && position.y >= 0, U"dialog fits viewport");
				canvas->setPositionScale(position, Vec2{ scale, scale });
				canvas->update(noco::HitTestEnabledYN::No);
				RenderTexture target{ size, ColorF{ .04 } };
				{
					const ScopedRenderTarget2D render{ target };
					canvas->draw();
				}
				Graphics2D::Flush();
				Image image;
				target.readAsImage(image);
				check(image.save(FsUtils::GetResourcePath(U"error-{}-{}x{}.png"_fmt(language, size.x, size.y))), U"dialog capture");
			}
		}
		Scene::Resize(originalSize);
		results.writeln(U"TOTAL {} error report checks passed"_fmt(checks));
	}
}
