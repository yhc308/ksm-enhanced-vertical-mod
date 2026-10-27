#pragma once
#include <Siv3D.hpp>
#include <exception>

namespace ErrorReporting
{
	enum class Stage
	{
		Game,
		CommandLine,
		Chart,
		Metadata,
		ExMaximum,
		Cache,
		Directory
	};
	enum class Reason
	{
		MissingFile,
		AccessDenied,
		PathTooLong,
		ChartFormat,
		Cache,
		Lookup,
		Unknown
	};

	struct Report
	{
		Stage stage = Stage::Game;
		Reason reason = Reason::Unknown;
		FilePath path;
		String detail;
		String id;
		bool logSaved = false;
		int32 skippedCharts = 0;
	};

	Report Record(Stage stage, FilePathView path, StringView detail, Reason reason = Reason::Unknown, int32 skippedCharts = 0);
	Report Record(Stage stage, FilePathView path, const std::exception& error);
	Report FileUnavailable(Stage stage, FilePathView path);
	String FormatMessage(const Report& report);
	String Code(Reason reason);
	FilePath LogPath(const Report& report);
	String ShortPath(FilePathView path);
}
