#pragma once
#include <Siv3D.hpp>
#include "AudioOutputRestartPolicy.hpp"
#include "Common/FsUtils.hpp"
#include <filesystem>
#include <fstream>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace AudioOutputRestart {
inline bool& Pending() { static bool value=false; return value; }
inline bool IsChild() { return System::GetCommandLineArgs().contains(U"--audio-output-restarted"); }
inline void Log(const String& message) {
    const auto utf8 = FsUtils::GetResourcePath(U"audio-output-switch.log").toUTF8();
    const std::filesystem::path path{std::u8string_view{reinterpret_cast<const char8_t*>(utf8.data()),utf8.size()}};
    std::ofstream stream{path,std::ios::app};
    stream << Time::GetMillisec() << " " << message.toUTF8() << std::endl;
}
inline bool ForceFixture() {
    const bool enabled=System::GetCommandLineArgs().contains(U"--audio-force-clean-restart");
    if (enabled && !FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Restart fixture requires an isolated test directory"};
    return enabled;
}
inline void WaitForPreviousProcess() {
#ifdef _WIN32
    if (!IsChild()) return;
    Optional<uint32> parentId;
    for (const auto& arg:System::GetCommandLineArgs()) {
        if (arg.starts_with(U"--audio-restart-parent=")) parentId=ParseOpt<uint32>(arg.substr(23));
    }
    if (!parentId || *parentId==0 || *parentId==GetCurrentProcessId()) throw Error{U"Invalid audio restart parent"};
    HANDLE parent=OpenProcess(SYNCHRONIZE,FALSE,*parentId);
    if (parent) {
        Log(U"restart child waiting for previous process {} to exit"_fmt(*parentId));
        const auto waited=WaitForSingleObject(parent,15000);
        CloseHandle(parent);
        if (waited!=WAIT_OBJECT_0) {
            Log(U"restart failed: previous process did not exit; output will not be opened concurrently");
            throw Error{U"The previous game did not exit. Close it before starting the game again."};
        }
    } else if (GetLastError()!=ERROR_INVALID_PARAMETER) {
        throw Error{U"Cannot verify previous process exit for audio restart"};
    }
    Log(U"previous process exited; allow device release to settle");
    Sleep(300);
#endif
}
// Configuration has already been saved by OptionScene. The child waits for normal
// parent shutdown, including ASIO/BASS cleanup, before initializing any output.
inline bool Launch() {
#ifdef _WIN32
    if (System::GetCommandLineArgs().contains(U"--audio-restart-fail-launch")) {
        if (!FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Restart failure fixture requires an isolated test directory"};
        Log(U"restart launch failed: injected fixture failure"); return false;
    }
    wchar_t module[32768]{};
    const DWORD length=GetModuleFileNameW(nullptr,module,32768);
    if (!length || length>=32768) return false;
    const std::wstring executable{module,length};
    std::wstring command=L"\""+executable+L"\" --audio-output-restarted --audio-restart-parent="+std::to_wstring(GetCurrentProcessId());
    if (System::GetCommandLineArgs().contains(U"--audio-restart-fixture")) {
        if (!FileSystem::Exists(FsUtils::GetResourcePath(U"audio-test-fixture"))) throw Error{U"Restart fixture requires an isolated test directory"};
        command+=L" --audio-options-smoke-test --audio-restart-fixture";
    }
    STARTUPINFOW startup{}; startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    const auto directory=std::filesystem::path{executable}.parent_path().wstring();
    if (!CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,directory.c_str(),&startup,&process)) {
        Log(U"restart launch failed: Win32 error {}"_fmt(GetLastError()));
        return false;
    }
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    Pending()=true;
    Log(U"restart child launched; exiting old ASIO process without reopening WASAPI here");
    System::Exit();
    return true;
#else
    return false;
#endif
}
}
