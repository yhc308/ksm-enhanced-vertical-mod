#pragma once
#include <Siv3D.hpp>
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#include <shellapi.h>
#include <memory>
#endif

namespace CommandLine
{
	inline const Array<String>& Arguments()
	{
		static const Array<String> arguments = []() -> Array<String>
		{
#ifdef _WIN32
			int count = 0;
			const std::unique_ptr<LPWSTR, decltype(&::LocalFree)> values{
				::CommandLineToArgvW(::GetCommandLineW(), &count), &::LocalFree
			};
			if (!values)
				throw Error{ U"Failed to read Unicode command line" };
			Array<String> result;
			result.reserve(count);
			for (int i = 0; i < count; ++i)
				result.push_back(Unicode::FromWstring(values.get()[i]));
			return result;
#else
			return System::GetCommandLineArgs();
#endif
		}();
		return arguments;
	}
}
