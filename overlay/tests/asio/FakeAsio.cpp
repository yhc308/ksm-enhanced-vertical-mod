// Test-only ASIO host. This DLL must never be packaged with the game.
#include <windows.h>
#define BASSASIODEF(f) __declspec(dllexport) WINAPI f
#include "../../ksmaudio/ThirdParty_Windows/bassasio/bassasio.h"
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
static ASIOPROC* render = nullptr;
static ASIONOTIFYPROC* notify = nullptr;
static void* notifyUser = nullptr;
static double rate = 44100;
static std::atomic_bool started{ false };
static std::thread autoPump;
static void StopAutoPump()
{
	started = false;
	if (autoPump.joinable()) autoPump.join();
}
static unsigned stopCalls = 0, rateSetCalls = 0;
static int error = 0;
static bool Mode(const wchar_t* expected)
{
	wchar_t mode[64]{};
	GetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", mode, 64);
	return wcscmp(mode, expected) == 0;
}
#define EXPORT extern "C" __declspec(dllexport)
EXPORT DWORD WINAPI BASS_ASIO_GetVersion()
{
	return 0x01040300;
}
EXPORT BOOL WINAPI BASS_ASIO_SetUnicode(BOOL)
{
	return TRUE;
}
EXPORT void WINAPI BASS_ASIO_SetWindow(HWND)
{
}
EXPORT DWORD WINAPI BASS_ASIO_ErrorGetCode()
{
	return error;
}
EXPORT BOOL WINAPI BASS_ASIO_GetDeviceInfo(DWORD index, BASS_ASIO_DEVICEINFO* info)
{
	if (index || Mode(L"missing"))
		return FALSE;
	info->name = reinterpret_cast<const char*>(L"KSM Mock ASIO");
	info->driver = "test";
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_Init(int, DWORD)
{
	stopCalls = 0;
	rateSetCalls = 0;
	rate = (Mode(L"48k") || Mode(L"locked48")) ? 48000 : ((Mode(L"bad-rate") || Mode(L"change-rate")) ? 96000 : 44100);
	if (Mode(L"busy"))
	{
		error = 3;
		return FALSE;
	}
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_Free()
{
	StopAutoPump();
	render = nullptr;
	notify = nullptr;
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_GetInfo(BASS_ASIO_INFO* info)
{
	*info = {};
	info->outputs = Mode(L"mono") ? 1 : 2;
	info->bufpref = 128;
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_SetRate(double value)
{
	++rateSetCalls;
	if (Mode(L"bad-rate") || Mode(L"locked48") || (Mode(L"48k") && value == 44100))
		return FALSE;
	rate = value;
	return TRUE;
}
EXPORT double WINAPI BASS_ASIO_GetRate()
{
	return Mode(L"invalid-readback") ? 0 : rate;
}
EXPORT BOOL WINAPI BASS_ASIO_Start(DWORD frames, DWORD)
{
	if (Mode(L"start-fail"))
	{
		error = 3;
		return FALSE;
	}
	started = true;
	if (Mode(L"silent-auto"))
	{
		const DWORD count = frames ? frames : 128;
		autoPump = std::thread([count]
		{
			std::vector<float> buffer(count * 2);
			auto next = std::chrono::steady_clock::now();
			while (started)
			{
				if (render) render(FALSE, 0, buffer.data(), count * 2 * sizeof(float), nullptr);
				next += std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(count / rate));
				std::this_thread::sleep_until(next);
			}
		});
	}
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_Stop()
{
	++stopCalls;
	StopAutoPump();
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_IsStarted()
{
	return started;
}
EXPORT DWORD WINAPI BASS_ASIO_GetLatency(BOOL)
{
	wchar_t value[32]{};
	if (GetEnvironmentVariableW(L"KSM_ASIO_TEST_LATENCY", value, 32))
		return static_cast<DWORD>(wcstoul(value, nullptr, 10));
	return 128;
}
EXPORT BOOL WINAPI BASS_ASIO_SetNotify(ASIONOTIFYPROC* proc, void* user)
{
	notify = proc;
	notifyUser = user;
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_ChannelEnable(BOOL, DWORD, ASIOPROC* proc, void*)
{
	render = proc;
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_ChannelJoin(BOOL, DWORD, int)
{
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_ChannelSetFormat(BOOL, DWORD, DWORD)
{
	return TRUE;
}
EXPORT BOOL WINAPI BASS_ASIO_ChannelSetRate(BOOL, DWORD, double)
{
	return TRUE;
}
EXPORT DWORD WINAPI FakeAsioRender(DWORD frames, float* buffer)
{
	return render ? render(FALSE, 0, buffer, frames * 2 * sizeof(float), nullptr) : 0;
}
EXPORT void WINAPI FakeAsioNotify(DWORD value)
{
	if (notify)
		notify(value, notifyUser);
}

EXPORT unsigned WINAPI FakeAsioStopCalls()
{
	return stopCalls;
}

EXPORT unsigned WINAPI FakeAsioRateSetCalls()
{
	return rateSetCalls;
}

// Identifies this sink to the diagnostic host. It never opens an OS audio output.
EXPORT BOOL WINAPI FakeAsioSilentOnly() { return TRUE; }
