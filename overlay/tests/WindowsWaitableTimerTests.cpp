#include <catch2/catch.hpp>
#if defined(_WIN32)
#include "Common/WindowsWaitableTimer.hpp"
#include <type_traits>

static_assert(!std::is_copy_constructible_v<WindowsWaitableTimer>);
static_assert(!std::is_copy_assignable_v<WindowsWaitableTimer>);
static_assert(!std::is_move_constructible_v<WindowsWaitableTimer>);

TEST_CASE("An unavailable timer reports the need for the existing sleep fallback")
{
	WindowsWaitableTimer timer{ nullptr };
	REQUIRE_FALSE(timer.waitUntil(std::chrono::steady_clock::now()));
	REQUIRE_FALSE(timer.waitUntil(std::chrono::steady_clock::now() + std::chrono::milliseconds(1)));
}

TEST_CASE("An expired waitable-timer deadline needs no wait")
{
	const HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_MODIFY_STATE | SYNCHRONIZE);
	REQUIRE(handle != nullptr);
	WindowsWaitableTimer timer{ handle };
	REQUIRE(timer.waitUntil(std::chrono::steady_clock::now() - std::chrono::milliseconds(1)));
}

TEST_CASE("A future waitable-timer deadline does not return early")
{
	const HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_MODIFY_STATE | SYNCHRONIZE);
	REQUIRE(handle != nullptr);
	WindowsWaitableTimer timer{ handle };
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
	REQUIRE(timer.waitUntil(deadline));
	REQUIRE(std::chrono::steady_clock::now() >= deadline);
}

TEST_CASE("The waitable-timer owner closes its native handle")
{
	const HANDLE handle = CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_MODIFY_STATE | SYNCHRONIZE);
	REQUIRE(handle != nullptr);
	DWORD flags = 0;
	{
		WindowsWaitableTimer timer{ handle };
		REQUIRE(GetHandleInformation(handle, &flags) != FALSE);
		REQUIRE(timer.waitUntil(std::chrono::steady_clock::now()));
	}
	REQUIRE(GetHandleInformation(handle, &flags) == FALSE);
}

TEST_CASE("A failed native timer operation reports fallback and still releases ownership")
{
	const HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	REQUIRE(event != nullptr);
	{
		WindowsWaitableTimer timer{ event };
		REQUIRE_FALSE(timer.waitUntil(std::chrono::steady_clock::now() + std::chrono::milliseconds(1)));
	}
	DWORD flags = 0;
	REQUIRE(GetHandleInformation(event, &flags) == FALSE);
}

TEST_CASE("The native high-resolution timer works on the validated Windows host")
{
	WindowsWaitableTimer timer;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
	REQUIRE(timer.waitUntil(deadline));
	REQUIRE(std::chrono::steady_clock::now() >= deadline);
}
#endif
