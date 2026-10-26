#include "../src/InstallerCore.hpp"
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
	if (argc != 3 && argc != 4) return 2;
	const bool progressTest = argc == 4 && std::wstring(argv[3]) == L"--progress";
	try
	{
		Setup::Package package(argv[1]);
		Setup::Engine engine(package, argv[2]);
		std::atomic_bool cancel{false};
		engine.cancel = &cancel;
		int previous = -1, samples = 0;
		engine.progressValue = [&previous, &samples](int value)
		{
			if (value < previous || value < 0 || value >= 100) throw Setup::Error("progress_test", "Invalid progress before commit");
			previous = value; ++samples;
		};
		engine.progress = [&cancel, progressTest](const std::wstring& message)
		{
			if (!progressTest && message.rfind(L"1/", 0) == 0) cancel = true;
		};
		engine.Run("install");
		if (progressTest)
		{
			if (previous != 99 || samples < 3) return 6;
			previous = -1; samples = 0;
			engine.Run("remove");
			return previous == 99 && samples >= 3 ? 0 : 7;
		}
		return 3;
	}
	catch (const Setup::Error& error)
	{
		return error.code == "cancelled" ? 0 : 4;
	}
	catch (...) { return 5; }
}
