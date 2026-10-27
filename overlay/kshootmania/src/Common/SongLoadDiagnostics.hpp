#pragma once
#include "ErrorReporting.hpp"

namespace SongLoadDiagnostics
{
	inline thread_local Array<ErrorReporting::Report>* activeBatch = nullptr;

	class Batch
	{
		Array<ErrorReporting::Report>* m_previous;

	public:
		Array<ErrorReporting::Report> reports;
		Batch()
			: m_previous(activeBatch)
		{
			activeBatch = &reports;
		}
		~Batch()
		{
			activeBatch = m_previous;
		}
		Batch(const Batch&) = delete;
		Batch& operator=(const Batch&) = delete;
	};

	inline ErrorReporting::Stage StageFromLabel(StringView stage)
	{
		if (stage == U"chart metadata" || stage == U"select chart")
			return ErrorReporting::Stage::Metadata;
		if (stage.starts_with(U"EX"))
			return ErrorReporting::Stage::ExMaximum;
		if (stage == U"open directory")
			return ErrorReporting::Stage::Directory;
		return ErrorReporting::Stage::Cache;
	}

	inline ErrorReporting::Report Write(StringView stage, FilePathView path, StringView message)
	{
		const auto typedStage = StageFromLabel(stage);
		const auto reason = typedStage == ErrorReporting::Stage::Metadata || typedStage == ErrorReporting::Stage::ExMaximum
								? ErrorReporting::Reason::ChartFormat
							: typedStage == ErrorReporting::Stage::Cache
								? ErrorReporting::Reason::Cache
								: ErrorReporting::Reason::Unknown;
		auto report = ErrorReporting::Record(typedStage, path, message, reason);
		if (activeBatch)
			activeBatch->push_back(report);
		return report;
	}

	inline ErrorReporting::Report Write(StringView stage, FilePathView path, const std::exception& error)
	{
		auto report = ErrorReporting::Record(StageFromLabel(stage), path, error);
		if (activeBatch)
			activeBatch->push_back(report);
		return report;
	}
}
