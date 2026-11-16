// Diagnostic host: no desktop switch, foreground request, or global input.
#include <windows.h>
#include <string>
#include <iostream>

static std::wstring Quote(const std::wstring& value)
{
	std::wstring result = L"\"";
	for (const auto ch : value)
	{
		if (ch == L'\"') result += L'\\';
		result += ch;
	}
	return result + L"\"";
}

int wmain(int argc, wchar_t** argv)
{
	if (argc < 4) return 2;
	bool silent = false;
	for (int i = 4; i < argc; ++i) silent |= std::wstring{ argv[i] } == L"--silent-qa";
	if (!silent) return 3;
	const auto desktopName = L"KsmSilentQA-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
	const HDESK desktop = CreateDesktopW(desktopName.c_str(), nullptr, nullptr, 0, GENERIC_ALL, nullptr);
	if (!desktop) return 4;
	std::wstring command = Quote(argv[1]);
	for (int i = 4; i < argc; ++i) command += L" " + Quote(argv[i]);
	STARTUPINFOW info{};
	info.cb = sizeof(info);
	info.lpDesktop = const_cast<wchar_t*>(desktopName.c_str());
	info.dwFlags = STARTF_USEPOSITION | STARTF_USESHOWWINDOW;
	info.dwX = static_cast<DWORD>(-1800);
	info.dwY = 64;
	info.wShowWindow = SW_SHOWNOACTIVATE;
	PROCESS_INFORMATION process{};
	if (!CreateProcessW(argv[1], command.data(), nullptr, nullptr, FALSE,
		CREATE_UNICODE_ENVIRONMENT | BELOW_NORMAL_PRIORITY_CLASS, nullptr, argv[2], &info, &process))
	{
		CloseDesktop(desktop);
		return 5;
	}
	const DWORD timeout = static_cast<DWORD>(_wtoi(argv[3])) * 1000;
	const bool completed = WaitForSingleObject(process.hProcess, timeout) == WAIT_OBJECT_0;
	if (!completed)
	{
		// Only the process created above is owned by this host.
		TerminateProcess(process.hProcess, 91);
		WaitForSingleObject(process.hProcess, 5000);
	}
	DWORD exit = 0;
	GetExitCodeProcess(process.hProcess, &exit);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	CloseDesktop(desktop);
	std::cout << "privateDesktop=true, interactiveDesktopSwitched=false, inputInjected=false, exitCode=" << exit << '\n';
	return completed && exit == 0 ? 0 : 2;
}
