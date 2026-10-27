#include "ErrorReporting.hpp"
#include "FsUtils.hpp"
#include "I18n/I18n.hpp"
#include <atomic>
#include <filesystem>
#include <stdexcept>

namespace ErrorReporting
{
	namespace
	{
		String Text(StringView key, StringView fallback)
		{
			const StringView translated = I18n::GetByCategoryAndKey(U"Mod", String{ key });
			return translated.isEmpty() ? String{ fallback } : String{ translated };
		}

		StringView StageName(Stage stage)
		{
			switch (stage)
			{
			case Stage::CommandLine:
				return U"command-line";
			case Stage::Chart:
				return U"chart";
			case Stage::Metadata:
				return U"chart.metadata";
			case Stage::ExMaximum:
				return U"chart.ex-maximum";
			case Stage::Cache:
				return U"song.cache";
			case Stage::Directory:
				return U"song.directory";
			default:
				return U"game";
			}
		}

		String StageText(Stage stage)
		{
			switch (stage)
			{
			case Stage::CommandLine:
				return Text(U"ErrorStageCommandLine", U"Opening a chart from the command line");
			case Stage::Chart:
				return Text(U"ErrorStageChart", U"Reading chart data");
			case Stage::Metadata:
				return Text(U"ErrorStageMetadata", U"Reading chart information");
			case Stage::ExMaximum:
				return Text(U"ErrorStageEx", U"Calculating the EX maximum");
			case Stage::Cache:
				return Text(U"ErrorStageCache", U"Reading the song index");
			case Stage::Directory:
				return Text(U"ErrorStageDirectory", U"Opening a song folder");
			default:
				return Text(U"ErrorStageGame", U"Running the game");
			}
		}

		String ReasonText(Reason reason)
		{
			switch (reason)
			{
			case Reason::MissingFile:
				return Text(U"ErrorReasonMissing", U"The file or folder was not found.");
			case Reason::AccessDenied:
				return Text(U"ErrorReasonAccess", U"Access was denied.");
			case Reason::PathTooLong:
				return Text(U"ErrorReasonPath", U"The file system rejected the path length.");
			case Reason::ChartFormat:
				return Text(U"ErrorReasonChart", U"Chart data could not be read or validated.");
			case Reason::Cache:
				return Text(U"ErrorReasonCache", U"The song index could not be read.");
			case Reason::Lookup:
				return Text(U"ErrorReasonLookup", U"A required internal entry was not found.");
			default:
				return Text(U"ErrorReasonUnknown", U"The cause has not been identified. See the log.");
			}
		}
	}

	String Code(Reason reason)
	{
		switch (reason)
		{
		case Reason::MissingFile:
			return U"IO-001";
		case Reason::AccessDenied:
			return U"IO-002";
		case Reason::PathTooLong:
			return U"IO-003";
		case Reason::ChartFormat:
			return U"CHART-001";
		case Reason::Cache:
			return U"CACHE-001";
		case Reason::Lookup:
			return U"SEL-003";
		default:
			return U"APP-001";
		}
	}

	FilePath LogPath(const Report& report)
	{
		return FsUtils::GetResourcePath(report.stage == Stage::Game ? U"log/error.log" : U"log/song-loading.log");
	}

	String ShortPath(FilePathView path)
	{
		String normalized{ path };
		normalized.replace(U'\\', U'/');
		if (normalized.size() <= 64)
			return normalized;
		while (normalized.ends_with(U'/'))
			normalized.pop_back();
		const auto parts = normalized.split(U'/');
		if (parts.isEmpty())
			return normalized;
		const String last = parts.back();
		if (last.size() > 64)
		{
			const String extension = FileSystem::Extension(last);
			return U"…/" + Text(U"ErrorLongFilename", U"[Long filename; see log]") + (extension.size() <= 12 ? U"." + extension : U"");
		}
		if (parts.size() < 2)
			return last;
		const String pair = parts[parts.size() - 2] + U"/" + last;
		// Elide complete path components, never split an emoji or combining sequence.
		return U"…/" + (pair.size() <= 64 ? pair : last);
	}

	Report Record(Stage stage, FilePathView path, StringView detail, Reason reason, int32 skippedCharts)
	{
		static std::atomic<uint64> sequence{ 0 };
		Report report{ stage, reason, FilePath{ path }, String{ detail }, U"{}-{}-{}"_fmt(DateTime::Now().format(U"yyyyMMdd-HHmmss"), Time::GetMicrosec(), ++sequence), false, skippedCharts };
		try
		{
			const FilePath destination = LogPath(report);
			FileSystem::CreateDirectories(FileSystem::ParentPath(destination));
			const String entry = U"[{}] id={} code={} stage={} skipped={}\npath={}\nexecutable={}\ndetail={}\n"_fmt(
				DateTime::Now().format(), report.id, Code(reason), StageName(stage), skippedCharts, path, FileSystem::ModulePath(), detail);
			const std::string bytes = entry.toUTF8();
			BinaryWriter writer{ destination, OpenMode::Append };
			if (writer)
			{
				writer.write(bytes.data(), bytes.size());
				writer.close();
				BinaryReader verify{ destination };
				if (verify && verify.size() >= static_cast<int64>(bytes.size()))
				{
					verify.setPos(verify.size() - bytes.size());
					std::string tail(bytes.size(), '\0');
					report.logSaved = verify.read(tail.data(), tail.size()) == static_cast<int64>(tail.size()) && tail == bytes;
				}
			}
		}
		catch (const Error&)
		{
		}
		catch (const std::exception&)
		{
		}
		return report;
	}

	Report Record(Stage stage, FilePathView path, const std::exception& error)
	{
		Reason reason = Reason::Unknown;
		if (dynamic_cast<const std::out_of_range*>(&error))
			reason = Reason::Lookup;
		else if (const auto fileError = dynamic_cast<const std::filesystem::filesystem_error*>(&error))
		{
			const auto code = fileError->code();
			if (code == std::errc::no_such_file_or_directory)
				reason = Reason::MissingFile;
			else if (code == std::errc::permission_denied)
				reason = Reason::AccessDenied;
			else if (code == std::errc::filename_too_long)
				reason = Reason::PathTooLong;
		}
		if (reason == Reason::Unknown && stage == Stage::Cache)
			reason = Reason::Cache;
		return Record(stage, path, Unicode::FromUTF8(error.what()), reason);
	}

	Report FileUnavailable(Stage stage, FilePathView path)
	{
		std::error_code error;
		const auto status = std::filesystem::status(std::filesystem::path{ String{ path }.toUTF32() }, error);
		Reason reason = Reason::Unknown;
		if (error == std::errc::permission_denied)
			reason = Reason::AccessDenied;
		else if (error == std::errc::filename_too_long)
			reason = Reason::PathTooLong;
		else if (error == std::errc::no_such_file_or_directory || (!error && !std::filesystem::exists(status)))
			reason = Reason::MissingFile;
		return Record(stage, path, U"File unavailable (system code {})"_fmt(error.value()), reason);
	}

	String FormatMessage(const Report& report)
	{
		const String title = report.skippedCharts > 0
								 ? Text(U"ErrorPartial", U"Some charts were skipped.")
								 : Text(U"ErrorSummary", U"An error occurred.");
		String body = title + U" [" + Code(report.reason) + U"]\n";
		body += Text(U"ErrorLabelStage", U"Step") + U": " + StageText(report.stage) + U"\n";
		if (!report.path.isEmpty())
			body += Text(U"ErrorLabelTarget", U"Target") + U": " + ShortPath(report.path) + U"\n";
		body += Text(U"ErrorLabelReason", U"Error") + U": " + ReasonText(report.reason) + U"\n";
		if (report.reason == Reason::Unknown && !report.detail.isEmpty())
		{
			const auto lines = report.detail.split(U'\n');
			if (!lines.isEmpty())
			{
				const String firstLine = lines.front().replaced(U'\r', U' ');
				if (firstLine.size() <= 120)
					body += firstLine + U"\n";
			}
		}
		if (report.skippedCharts > 0)
			body += Text(U"ErrorLabelSkipped", U"Skipped charts") + U": " + Format(report.skippedCharts) + U"\n";
		body += report.logSaved
					? Text(U"ErrorLabelLog", U"Log") + U": " + (report.stage == Stage::Game ? U"log/error.log" : U"log/song-loading.log")
					: Text(U"ErrorLogFailed", U"The detailed log could not be saved.");
		return body + U"\nID: " + report.id;
	}
}
