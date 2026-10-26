#pragma once
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <chrono>

class WindowsWaitableTimer
{
	HANDLE m_handle = nullptr;

public:
	WindowsWaitableTimer() noexcept
		: WindowsWaitableTimer(CreateWaitableTimerExW(nullptr, nullptr,
													  CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_MODIFY_STATE | SYNCHRONIZE))
	{
	}
	explicit WindowsWaitableTimer(HANDLE handle) noexcept
		: m_handle(handle)
	{
	}
	WindowsWaitableTimer(const WindowsWaitableTimer&) = delete;
	WindowsWaitableTimer& operator=(const WindowsWaitableTimer&) = delete;
	~WindowsWaitableTimer()
	{
		if (m_handle)
		{
			CloseHandle(m_handle);
		}
	}
	bool waitUntil(std::chrono::steady_clock::time_point deadline) const noexcept
	{
		if (!m_handle)
		{
			return false;
		}
		for (;;)
		{
			const auto remaining = deadline - std::chrono::steady_clock::now();
			if (remaining <= std::chrono::steady_clock::duration::zero())
			{
				return true;
			}
			using TimerDuration = std::chrono::duration<LONGLONG, std::ratio<1, 10000000>>;
			LARGE_INTEGER dueTime{};
			dueTime.QuadPart = -std::chrono::ceil<TimerDuration>(remaining).count();
			if (!SetWaitableTimer(m_handle, &dueTime, 0, nullptr, nullptr, FALSE) || WaitForSingleObject(m_handle, INFINITE) != WAIT_OBJECT_0)
			{
				return false;
			}
		}
	}
};
#endif
