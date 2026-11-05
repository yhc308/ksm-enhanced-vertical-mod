// Test-only: derive source position from PCM contents, not production clock helpers.
#include "ksmaudio/ksmaudio.hpp"
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using Clock = std::chrono::steady_clock;
static double Now()
{
	return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}
static void Ramp(unsigned rate)
{
	std::ofstream f("clock-ramp.wav", std::ios::binary);
	unsigned frames = rate * 5, bytes = frames * 8, size = 36 + bytes, fmtSize = 16, byteRate = rate * 8;
	unsigned short fmt = 3, channels = 2, align = 8, bits = 32;
	f.write("RIFF", 4);
	f.write((char*)&size, 4);
	f.write("WAVEfmt ", 8);
	f.write((char*)&fmtSize, 4);
	f.write((char*)&fmt, 2);
	f.write((char*)&channels, 2);
	f.write((char*)&rate, 4);
	f.write((char*)&byteRate, 4);
	f.write((char*)&align, 2);
	f.write((char*)&bits, 2);
	f.write("data", 4);
	f.write((char*)&bytes, 4);
	for (unsigned i = 0; i < frames; i++)
	{
		float value = (i + 1.0f) / rate / 10;
		f.write((char*)&value, 4);
		f.write((char*)&value, 4);
	}
}
int main()
{
	try
	{
		unsigned checks = 0;
		for (const auto* mode : { L"", L"48k" })
			for (unsigned block : { 64, 128, 256, 512, 1024 })
			{
				const unsigned rate = *mode ? 48000 : 44100;
				SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", mode);
				const auto latency = std::to_wstring(block * 2 + 40);
				SetEnvironmentVariableW(L"KSM_ASIO_TEST_LATENCY", latency.c_str());
				Ramp(rate);
				if (!ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO") || !ksmaudio::IsASIOActive())
					throw std::runtime_error("mock init");
				auto render = (DWORD(WINAPI*)(DWORD, float*))GetProcAddress(GetModuleHandleW(L"bassasio.dll"), "FakeAsioRender");
				if (!render)
					throw std::runtime_error("missing fake render");
				{
					ksmaudio::Stream source("clock-ramp.wav");
					source.seekPosSec(ksmaudio::SecondsF{ .5 });
					source.play();
					std::vector<float> samples(block * 2);
					for (int repeat = 0; repeat < 3; repeat++)
						for (double phase : { 0., .25, .5, .9, 1.5, 2. })
						{
							for (int warm = 0; warm < 8; warm++)
								render(block, samples.data());
							const double callbackBefore = Now();
							render(block, samples.data());
							const double callbackAfter = Now();
							// Last PCM sample encodes its one-based source frame. This is an independent
							// witness of which source sample was actually submitted to the output.
							const double pcmEnd = samples.back() * 10.;
							const double target = callbackAfter + phase * block / rate;
							while (Now() < target)
								YieldProcessor();
							const double queryBefore = Now();
							const auto c = source.clockSnapshot();
							const double queryAfter = Now();
							const auto elapsedMin = std::clamp((queryBefore - callbackAfter) * rate, 0., double(block));
							const auto elapsedMax = std::clamp((queryAfter - callbackBefore) * rate, 0., double(block));
							const double earliest = pcmEnd - (block * 2 + 40.) / rate + elapsedMin / rate;
							const double latest = pcmEnd - (block * 2 + 40.) / rate + elapsedMax / rate;
							const double tolerance = 2. / rate;
							if (!c.valid || c.audiblePositionSec < earliest - tolerance || c.audiblePositionSec > latest + tolerance)
							{
								std::cerr << "rate=" << rate << ", block=" << block << ", phase=" << phase << ", pcm=" << pcmEnd << ", expected=[" << earliest << ',' << latest << "], actual=" << c.audiblePositionSec << '\n';
								throw std::runtime_error("PCM clock phase mismatch");
							}
							++checks;
						}
					source.pause();
					const double paused = source.posSec().count();
					render(block, samples.data());
					if (source.posSec().count() != paused)
						throw std::runtime_error("pause changed");
					++checks;
					source.stop();
					source.play();
					if (source.posSec().count() != 0)
						throw std::runtime_error("restart stale callback phase");
					++checks;
				}
				ksmaudio::Terminate();
			}
		SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", nullptr);
		SetEnvironmentVariableW(L"KSM_ASIO_TEST_LATENCY", nullptr);
		std::cout << "PCM CLOCK PHASE " << checks << " checks passed\n";
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		ksmaudio::Terminate();
		return 1;
	}
}
