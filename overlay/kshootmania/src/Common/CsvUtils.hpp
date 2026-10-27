#pragma once
#include <Siv3D.hpp>

namespace CsvUtils
{
	inline Optional<Array<String>> ParseRow(StringView row)
	{
		Array<String> fields;
		String field;
		bool quoted = false, wasQuoted = false, closed = false;
		for (size_t i = 0; i < row.size(); ++i)
		{
			const char32 ch = row[i];
			if (quoted)
			{
				if (ch == U'"')
				{
					if (i + 1 < row.size() && row[i + 1] == U'"')
					{
						field.push_back(U'"');
						++i;
					}
					else
					{
						quoted = false;
						closed = true;
					}
				}
				else
					field.push_back(ch);
			}
			else if (ch == U',')
			{
				fields.push_back(wasQuoted ? field : field.trimmed());
				field.clear();
				wasQuoted = false;
				closed = false;
			}
			else if (ch == U'"' && field.trimmed().isEmpty() && !closed)
			{
				field.clear();
				quoted = true;
				wasQuoted = true;
			}
			else if (closed)
			{
				if (ch != U' ' && ch != U'\t')
					return none;
			}
			else
				field.push_back(ch);
		}
		if (quoted)
			return none;
		fields.push_back(wasQuoted ? field : field.trimmed());
		return fields;
	}
}
