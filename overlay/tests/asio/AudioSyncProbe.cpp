#include "ksmaudio/ksmaudio.hpp"
#include "ksmaudio/StreamWithEffects.hpp"
#include <windows.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <stdexcept>
using Clock = std::chrono::steady_clock;
static double Now()
{
	return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}
static void Wave(const char* path)
{
	const unsigned frames = 44100 * 360, bytes = frames * 4;
	std::ofstream f(path, std::ios::binary);
	unsigned value = 36 + bytes;
	f.write("RIFF", 4);
	f.write((char*)&value, 4);
	f.write("WAVEfmt ", 8);
	value = 16;
	f.write((char*)&value, 4);
	unsigned short format = 1, ch = 2, align = 4, bits = 16;
	f.write((char*)&format, 2);
	f.write((char*)&ch, 2);
	value = 44100;
	f.write((char*)&value, 4);
	value *= 4;
	f.write((char*)&value, 4);
	f.write((char*)&align, 2);
	f.write((char*)&bits, 2);
	f.write("data", 4);
	f.write((char*)&bytes, 4);
	for (unsigned i = 0; i < frames; i++)
	{
		short v = i % 22050 < 44 ? 4096 : 0;
		f.write((char*)&v, 2);
		f.write((char*)&v, 2);
	}
}
struct Row
{
	double qpc, audible, render;
	std::uint64_t frame;
	bool playing;
};
int main(int argc, char** argv)
{
	try
	{
		if (argc < 4)
			throw std::runtime_error("backend seconds csv [driver]");
		const std::string mode = argv[1];
		int seconds = std::stoi(argv[2]);
		if (seconds < 1 || seconds > 360)
			throw std::runtime_error("duration");
		Wave("sync-probe.wav");
		ksmaudio::SetMasterVolume(0);
		if (!ksmaudio::Init(nullptr, mode == "asio" ? ksmaudio::AudioBackend::ASIO : ksmaudio::AudioBackend::Default, argc > 4 ? argv[4] : ""))
			throw std::runtime_error("init failed");
		const auto device = ksmaudio::GetAudioOutputStatus();
		if (mode == "asio" && !ksmaudio::IsASIOActive())
			throw std::runtime_error("ASIO fallback rejected");
		std::cout << std::setprecision(12) << "backend=" << mode << ", driver=" << device.driver << ", rate=" << device.sampleRate << ", frames=" << device.bufferSamples << ", latencyMs=" << device.latencySec * 1000 << "\n";
		std::vector<Row> rows;
		rows.reserve(seconds * 5000);
		const HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, 2, TIMER_ALL_ACCESS);
		if (!timer)
			throw std::runtime_error("high resolution timer unavailable");
		{
			ksmaudio::StreamWithEffects source("sync-probe.wav", 1, true, true);
			source.emplaceAudioEffectBusFX();
			source.emplaceAudioEffectBusLaser();
			source.play();
			const double start = Now();
			while (Now() - start < seconds)
			{
				const double before = Now();
				const auto c = source.clockSnapshot();
				const double after = Now();
				rows.push_back({ (before + after) * .5, c.audiblePositionSec, c.renderPositionSec, c.outputFrame, c.playing });
				LARGE_INTEGER due;
				due.QuadPart = -2500;
				SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0);
				WaitForSingleObject(timer, 1000);
			}
		}
		CloseHandle(timer);
		ksmaudio::Terminate();
		std::ofstream csv(argv[3]);
		csv << "qpc,audible,render,output_frame,playing\n"
			<< std::setprecision(15);
		for (const auto& row : rows)
			csv << row.qpc << ',' << row.audible << ',' << row.render << ',' << row.frame << ',' << row.playing << '\n';
		std::cout << "rows=" << rows.size() << ", seconds=" << seconds << ", physicallySilent=true\n";
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
