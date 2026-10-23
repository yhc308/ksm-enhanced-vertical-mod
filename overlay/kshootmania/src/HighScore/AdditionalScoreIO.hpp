#pragma once
#include <Siv3D.hpp>
#include "KscKey.hpp"
#include "MusicGame/PlayResult.hpp"

namespace AdditionalScoreIO
{
	using RecordMap = HashTable<String, int64>;

	/// Read an EX record without requiring the optional maximum from newer files.
	Optional<std::pair<String, int64>> ParseRecord(const String& line);
	RecordMap Read(FilePathView originalScorePath);
	bool Write(FilePathView originalScorePath, const MusicGame::PlayResult& result, const KscKey& condition);
}
