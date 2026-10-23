#include "AdditionalScoreIO.hpp"
#include "Common/FsUtils.hpp"
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif

namespace AdditionalScoreIO
{
	namespace
	{
		FilePath GetAdditionalFilePath(FilePathView path)
		{
			return FsUtils::EliminateExtension(path) + U"_additional.ksc";
		}

	}

	Optional<std::pair<String, int64>> ParseRecord(const String& line)
	{
		const auto separator = line.indexOf(U'=');
		if (separator == String::npos)
			return none;
		const auto fields = line.substr(separator + 1).split(U',');
		constexpr size_t kExScoreIndex = 9;
		if ((fields.size() != 10 && fields.size() != 11) || fields[kExScoreIndex].isEmpty())
			return none;
		if (const auto score = ParseOpt<int64>(fields[kExScoreIndex]); score && *score >= 0)
			return std::pair<String, int64>{ line.substr(0, separator), *score };
		return none;
	}

	RecordMap Read(FilePathView originalScorePath)
	{
		RecordMap records;
		TextReader reader{ GetAdditionalFilePath(originalScorePath) };
		String line;
		while (reader && reader.readLine(line))
		{
			if (const auto record = ParseRecord(line))
				records[record->first] = record->second;
		}
		return records;
	}

	bool Write(FilePathView path, const MusicGame::PlayResult& result, const KscKey& condition)
	{
		auto values = Read(path);
		if (result.exMaximum > 0 && result.exScore >= 0 && result.exScore <= result.exMaximum)
		{
			auto it = values.find(condition.toString());
			const int64 previous = it == values.end() || it->second > result.exMaximum ? 0 : it->second;
			values[condition.toString()] = Max(previous, result.exScore);
		}
		TextReader reader{ path };
		if (!reader)
			return false;
		String content, line;
		while (reader.readLine(line))
		{
			if (line.isEmpty())
				continue;
			const auto eq = line.indexOf(U'=');
			if (eq == String::npos)
				continue;
			const auto it = values.find(line.substr(0, eq));
			content += line + U"," + (it == values.end() ? String{} : Format(it->second)) + U"," + (result.exMaximum > 0 ? Format(result.exMaximum) : String{}) + U"\n";
		}
		const auto destination = GetAdditionalFilePath(path);
		const auto temporary = destination + U".{}.tmp"_fmt(Time::GetMicrosec());
		{
			TextWriter writer{ temporary, TextEncoding::UTF8_NO_BOM };
			if (!writer)
				return false;
			writer.write(content);
		}
		{
			TextReader verify{ temporary };
			if (!verify || verify.readAll() != content)
			{
				FileSystem::Remove(temporary);
				return false;
			}
		}
#ifdef _WIN32
		const bool ok = ::MoveFileExW(temporary.toWstr().c_str(), destination.toWstr().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
		const bool ok = FileSystem::Rename(temporary, destination);
#endif
		if (!ok)
			FileSystem::Remove(temporary);
		return ok;
	}
}
