#include "ksmaudio/ksmaudio.hpp"
#include "../../kshootmania/src/Common/AudioOutputRestartPolicy.hpp"
#include "../../ksmaudio/src/OutputResources.hpp"
#include <windows.h>
#include <cmath>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
using ksmaudio::Stream;
using ksmaudio::Sample;
static unsigned checks = 0;
void Check(bool condition, const char* description)
{
	if (!condition)
		throw std::runtime_error(description);
	++checks;
	std::cout << "PASS " << description << '\n';
}
void Near(float actual, float expected, const char* description)
{
	Check(std::abs(actual - expected) < 0.006f, description);
}
void WAV(const char* name, unsigned frames = 88200)
{
	std::ofstream f(name, std::ios::binary);
	unsigned size = 36 + frames * 4, formatSize = 16, sr = 44100, byteRate = 176400, dataSize = frames * 4;
	unsigned short format = 1, channels = 2, align = 4, bits = 16;
	f.write("RIFF", 4);
	f.write((char*)&size, 4);
	f.write("WAVEfmt ", 8);
	f.write((char*)&formatSize, 4);
	f.write((char*)&format, 2);
	f.write((char*)&channels, 2);
	f.write((char*)&sr, 4);
	f.write((char*)&byteRate, 4);
	f.write((char*)&align, 2);
	f.write((char*)&bits, 2);
	f.write("data", 4);
	f.write((char*)&dataSize, 4);
	for (unsigned i = 0; i < frames; ++i)
	{
		short left = 8192, right = 16384;
		f.write((char*)&left, 2);
		f.write((char*)&right, 2);
	}
}
int main(int argc, char** argv)
{
	try
	{
		if (argc > 1 && std::string(argv[1]) == "--restart-policy")
		{
			ksmaudio::AudioOutputStatus current;
			current.backend = ksmaudio::AudioBackend::ASIO;
			for (const auto& name : { "FiiO ASIO Driver", "FIIO USB ASIO", "fiio driver" })
			{
				current.driver = name;
				Check(AudioOutputRestart::Required(current, ksmaudio::AudioBackend::Default), "known FiiO ASIO exit skips same-process WASAPI initialization");
				Check(!AudioOutputRestart::Required(current, ksmaudio::AudioBackend::DirectSound), "DirectSound selection remains live");
				Check(!AudioOutputRestart::Required(current, ksmaudio::AudioBackend::ASIO), "ASIO-to-ASIO selection remains live");
			}
			current.driver = "TOPPING Pro USB Audio Device";
			Check(!AudioOutputRestart::Required(current, ksmaudio::AudioBackend::Default), "unaffected ASIO drivers keep live WASAPI switching");
			current.driver = "FiiO ASIO Driver";
			current.backend = ksmaudio::AudioBackend::Default;
			Check(!AudioOutputRestart::Required(current, ksmaudio::AudioBackend::Default), "native-to-native changes do not restart");
			std::cout << "RESTART POLICY " << checks << " checks passed" << std::endl;
			return 0;
		}
		if (argc > 1 && std::string(argv[1]) == "--list")
		{
			for (const auto& name : ksmaudio::GetASIODrivers())
				std::cout << name << '\n';
			return 0;
		}
		if (argc > 2 && std::string(argv[1]) == "--probe-driver")
		{
			const bool initialized = ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, argv[2]);
			Sleep(2500); // Silence only: no streams or samples are loaded in this branch.
			const auto status = ksmaudio::GetAudioOutputStatus();
			const auto failure = ksmaudio::GetAudioOutputFailure();
			std::cout << "initialized=" << initialized << ", ASIO=" << ksmaudio::IsASIOActive()
					  << ", driver=" << status.driver << ", sampleRate=" << status.sampleRate
					  << ", bufferSamples=" << status.bufferSamples << ", reportedLatencyMs=" << status.latencySec * 1000
					  << ", warning=" << status.warning << ", failure=" << failure << '\n';
			const bool healthy = initialized && ksmaudio::IsASIOActive() && failure.empty();
			ksmaudio::Terminate();
			return healthy ? 0 : 2;
		}
		if (argc > 1 && std::string(argv[1]) == "--rate-policy")
		{
			for (const auto& mode : { L"", L"48k", L"locked48", L"change-rate" })
			{
				SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", mode);
				Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "rate policy initialized");
				Check(ksmaudio::IsASIOActive(), "rate policy retains ASIO");
				auto count = (unsigned(WINAPI*)())GetProcAddress(GetModuleHandleW(L"bassasio.dll"), "FakeAsioRateSetCalls");
				Check(count != nullptr, "rate test counter available");
				const bool change = wcscmp(mode, L"change-rate") == 0;
				Check(count() == (change ? 1u : 0u), "usable current clock is never unnecessarily reset");
				const double expected = (wcscmp(mode, L"48k") == 0 || wcscmp(mode, L"locked48") == 0) ? 48000 : 44100;
				Check(ksmaudio::GetAudioOutputStatus().sampleRate == expected, "actual hardware rate is retained and reported");
				ksmaudio::Terminate();
			}
			SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", L"invalid-readback");
			ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO");
			Check(!ksmaudio::IsASIOActive(), "invalid rate readback cannot become an active ASIO clock");
			Check(!ksmaudio::GetAudioOutputStatus().warning.empty(), "invalid readback retains a diagnostic");
			ksmaudio::Terminate();
			SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", L"");
			std::cout << "RATE POLICY " << checks << " checks passed\n";
			return 0;
		}
		if (argc > 1 && std::string(argv[1]) == "--teardown-stress")
		{
			WAV("test-teardown.wav");
			Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "teardown ASIO initialized");
			auto render = (DWORD(WINAPI*)(DWORD, float*))GetProcAddress(GetModuleHandleW(L"bassasio.dll"), "FakeAsioRender");
			std::atomic<bool> stop{ false };
			std::atomic<unsigned> callbacks{ 0 }, errors{ 0 };
			std::thread callback([&]
								 {
									 std::vector<float> data(128);
									 while (!stop)
									 {
										 if (render(64, data.data()) != 512)
											 ++errors;
										 ++callbacks;
										 Sleep(1);
									 }
								 });
			bool failed = false;
			try
			{
				for (unsigned i = 0; i < 150; ++i)
				{
					ksmaudio::StreamWithEffects source("test-teardown.wav", 0.5, false, true, (i % 2) ? 1.25 : 1.0);
					auto* bus = source.emplaceAudioEffectBusFX();
					bus->emplaceAudioEffect<ksmaudio::Gate>("teardown-gate");
					source.play();
					Sleep(1);
					{
						Sample voice("test-teardown.wav", 2);
						voice.play(0.1);
						Sleep(1);
					}
					// Destruct buses, native DSP, tempo source and mixer stream during live rendering.
				}
			}
			catch (...)
			{
				failed = true;
			}
			stop = true;
			callback.join();
			Check(!failed, "150 concurrent DSP/tempo/stream/sample lifecycles completed");
			Check(callbacks > 100, "render callback continues across resource churn");
			Check(errors == 0 && ksmaudio::GetAudioOutputFailure().empty(), "resource churn has no callback failures");
			std::vector<float> silence(128);
			render(64, silence.data());
			bool quiet = true;
			for (float sample : silence)
				if (std::abs(sample) > 0.001f)
					quiet = false;
			Check(quiet, "all freed sources are silent with no stale DSP callbacks");
			ksmaudio::Terminate();
			std::cout << "TEARDOWN " << checks << " checks passed\n";
			return 0;
		}

		if (argc > 1 && std::string(argv[1]) == "--concurrency")
		{
			WAV("test-thread.wav");
			Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "concurrency ASIO initialized");
			auto render = (DWORD(WINAPI*)(DWORD, float*))GetProcAddress(GetModuleHandleW(L"bassasio.dll"), "FakeAsioRender");
			{
				Stream source("test-thread.wav", 1, false, true, true);
				source.play();
				std::atomic<bool> stop{ false };
				std::atomic<unsigned> callbacks{ 0 }, errors{ 0 };
				std::thread callback([&]
									 {
										 std::vector<float> data(128);
										 while (!stop.load())
										 {
											 if (render(64, data.data()) != 512)
												 ++errors;
											 ++callbacks;
											 Sleep(1);
										 }
									 });
				for (int i = 0; i < 500; ++i)
				{
					source.lockBegin();
					source.seekPosSec(ksmaudio::SecondsF{ (i % 100) * 0.001 });
					source.setVolume(0.5);
					source.play();
					Sleep(1);
					source.lockEnd();
					Sleep(1);
				}
				stop = true;
				callback.join();
				Check(callbacks > 100, "callback continues through preview-style locked seeks and FX updates");
				Check(errors == 0 && ksmaudio::GetAudioOutputFailure().empty(), "no callback errors or deadlock under concurrent transport");
			}
			ksmaudio::Terminate();
			std::cout << "CONCURRENCY " << checks << " checks passed" << std::endl;
			return 0;
		}
		if (argc > 1 && std::string(argv[1]) == "--latency-clock")
		{
			WAV("test-clock.wav");
			for (const auto* mode : { L"", L"48k" })
				for (const auto* latency : { L"64", L"128", L"512" })
				{
					SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", mode);
					SetEnvironmentVariableW(L"KSM_ASIO_TEST_LATENCY", latency);
					Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "clock sweep initialized");
					auto module = GetModuleHandleW(L"bassasio.dll");
					auto render = (DWORD(WINAPI*)(DWORD, float*))GetProcAddress(module, "FakeAsioRender");
					std::vector<float> samples(128);
					{
						Stream source("test-clock.wav");
						source.seekPosSec(ksmaudio::SecondsF{ 0.7 });
						source.play();
						Check(source.posSec().count() >= 0.6999, "seek origin valid before history warms");
						for (int i = 0; i < 30; ++i)
							render(64, samples.data());
						const auto c = source.clockSnapshot();
						const auto device = ksmaudio::GetAudioLatencySnapshot();
						std::cout << "CLOCK rate=" << device.output.sampleRate << ", latency=" << device.output.latencySec << ", render=" << c.renderPositionSec << ", audible=" << c.audiblePositionSec << ", difference=" << c.renderPositionSec - c.audiblePositionSec << std::endl;
						Check(std::abs((c.renderPositionSec - c.audiblePositionSec) - device.output.latencySec) < 0.0001, "driver latency applied once across rates and delays");
						source.pause();
						const auto position = source.posSec();
						for (int i = 0; i < 10; ++i)
							render(64, samples.data());
						Check(source.posSec() == position, "pause holds across delayed callbacks");
						source.play();
						Check(source.posSec().count() >= position.count() - 0.0001, "resume keeps audible origin");
						source.stop();
						source.play();
						Check(source.posSec().count() == 0, "restart clears prior seek history");
					}
					ksmaudio::Terminate();
				}
			SetEnvironmentVariableW(L"KSM_ASIO_TEST_MODE", nullptr);
			SetEnvironmentVariableW(L"KSM_ASIO_TEST_LATENCY", nullptr);
			std::cout << "CLOCK SWEEP " << checks << " checks passed" << std::endl;
			return 0;
		}
		if (argc > 3 && std::string(argv[1]) == "--soak-driver")
		{
			const std::string driver = argv[2];
			const int seconds = std::stoi(argv[3]);
			Check(seconds > 0 && seconds <= 1800, "bounded soak duration");
			ksmaudio::SetMasterVolume(0); // Decode/mix real sources; physically silent.
			Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, driver), "soak initialized");
			Check(ksmaudio::IsASIOActive(), "soak uses actual ASIO");
			const std::string assets = argc > 4 ? argv[4] : ".";
			{
				Stream bgm(assets + "/se/option_bgm.ogg", 1, false, true, true);
				Check(bgm.numChannels() > 0, "soak menu BGM loaded");
				bgm.play();
				Sample tick(assets + "/se/tick.wav", 8);
				const auto start = std::chrono::steady_clock::now();
				std::uint64_t previousFrame = 0;
				int nextTick = 0, nextReport = 0;
				while (std::chrono::steady_clock::now() - start < std::chrono::seconds(seconds))
				{
					const int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count());
					const auto metrics = ksmaudio::GetAudioLatencySnapshot();
					const auto failure = ksmaudio::GetAudioOutputFailure();
					if (!failure.empty())
						throw std::runtime_error(failure);
					if (elapsed >= nextTick)
					{
						Check(tick.schedulePlay(metrics.outputFrame + metrics.output.bufferSamples * 2), "soak event queued");
						nextTick = elapsed + 1;
					}
					if (elapsed >= nextReport)
					{
						if (elapsed > 0)
							Check(metrics.outputFrame > previousFrame, "physical callback advances");
						previousFrame = metrics.outputFrame;
						nextReport = elapsed + 60;
						std::cout << "SOAK elapsed=" << elapsed << ", outputFrames=" << metrics.outputFrame << ", bgm=" << bgm.posSec().count()
								  << ", rate=" << metrics.output.sampleRate << ", buffer=" << metrics.output.bufferSamples
								  << ", late=" << metrics.lateEvents << ", rejected=" << metrics.rejectedEvents << std::endl;
					}
					Sleep(10);
				}
			}
			ksmaudio::Terminate();
			std::cout << "SOAK PASSED seconds=" << seconds << std::endl;
			return 0;
		}
		if (argc > 1 && std::string(argv[1]) == "--failure")
		{
			const bool initialized = ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO");
			Check(!ksmaudio::IsASIOActive(), "failed ASIO never remains active");
			Check(!ksmaudio::GetAudioOutputStatus().warning.empty(), "failure reason retained");
			std::cout << "Fallback BASS initialized=" << initialized << ", warning=" << ksmaudio::GetAudioOutputStatus().warning << '\n';
			ksmaudio::Terminate();
			return 0;
		}
		WAV("test-stereo.wav");
		WAV("test-short.wav", 256);
		Check(ksmaudio::GetASIODrivers().size() == 1, "ASIO enumeration");
		Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "ASIO initialization");
		Check(ksmaudio::IsASIOActive(), "active ASIO backend");
		Check(ksmaudio::GetAudioOutputStatus().latencySec > 0, "reported output latency");
		auto module = GetModuleHandleW(L"bassasio.dll");
		auto render = (DWORD(WINAPI*)(DWORD, float*))GetProcAddress(module, "FakeAsioRender");
		auto notify = (void(WINAPI*)(DWORD))GetProcAddress(module, "FakeAsioNotify");
		Check(render && notify, "test driver hooks");
		std::vector<float> buffer(256);
		auto pump = [&]
		{
			Check(render(128, buffer.data()) == 1024, "complete stereo output buffer");
		};
		pump();
		Near(buffer[200], 0, "empty mixer supplies silence");
		{
			Stream bgm("test-stereo.wav");
			Check(!bgm.isPlaying(), "new stream paused");
			bgm.play();
			pump();
			Near(buffer[200], 0.25f, "BGM left routing");
			Near(buffer[201], 0.5f, "BGM right routing");
			Check(bgm.posSec().count() >= 0, "audible position nonnegative");
			pump();
			pump();
			Check(bgm.posSec().count() > 0, "audible position advances");
			bgm.pause();
			pump();
			Near(buffer[200], 0, "pause silences source");
			Check(!bgm.isPlaying(), "paused status");
			bgm.play();
			pump();
			Near(buffer[200], 0.25f, "resume");
			ksmaudio::SetMasterVolume(0.5);
			pump();
			Near(buffer[200], 0.125f, "master volume applied once");
			ksmaudio::SetMute(true);
			pump();
			Near(buffer[200], 0, "master mute");
			ksmaudio::SetMute(false);
			ksmaudio::SetMasterVolume(1);
			bgm.setMuted(true);
			pump();
			Near(buffer[200], 0, "stream mute");
			bgm.setMuted(false);
			pump();
			Near(buffer[200], 0.25f, "stream unmute");
			bgm.setVolume(0.5);
			pump();
			pump();
			Near(buffer[200], 0.125f, "stream volume");
			bgm.setVolume(1);
			pump();
			pump();
			struct Quarter final : ksmaudio::AudioEffect::IAudioEffect
			{
				void process(float* p, std::size_t n) override
				{
					for (std::size_t i = 0; i < n; ++i)
						p[i] *= 0.25f;
				}
				void updateStatusByFX(const ksmaudio::AudioEffect::Status&, std::optional<std::size_t>) override
				{
				}
				void updateStatusByLaser(const ksmaudio::AudioEffect::Status&, bool) override
				{
				}
				void setParamValueSet(ksmaudio::AudioEffect::ParamID, const ksmaudio::AudioEffect::ValueSet&) override
				{
				}
				std::unordered_map<ksmaudio::AudioEffect::ParamID, ksmaudio::AudioEffect::ValueSet> paramValueSetDict() const override
				{
					return {};
				}
				void setBypass(bool) override
				{
				}
			} quarter;
			auto effect = bgm.addAudioEffect(&quarter, 10);
			Check(effect != 0, "DSP attached");
			pump();
			Near(buffer[200], 0.0625f, "DSP reaches ASIO output");
			bgm.removeAudioEffect(effect);
			bgm.seekPosSec(ksmaudio::SecondsF{ 1 });
			pump();
			pump();
			Check(bgm.posSec().count() > 0.9, "seek updates audible position");
			bgm.stop();
			pump();
			Near(buffer[200], 0, "stop");
			bgm.play();
			pump();
			Near(buffer[200], 0.25f, "play after stop");
			bgm.setFadeOut(ksmaudio::Duration{ 0.02 });
			Sleep(40);
			pump();
			pump();
			Near(buffer[200], 0, "fade out");
			bgm.setFadeIn(ksmaudio::Duration{ 0.02 });
			Sleep(40);
			pump();
			pump();
			Near(buffer[200], 0.25f, "fade in");
			bgm.pause();
		}
		{
			Sample se("test-short.wav", 1);
			se.play(0.5);
			pump();
			Near(buffer[200], 0.125f, "effect sound volume");
			pump();
			pump();
			Near(buffer[200], 0, "effect sound ends");
			se.play();
			pump();
			Near(buffer[200], 0.25f, "effect sound replays after end");
		}
		pump();
		Near(buffer[200], 0, "destroyed sounds detached");
		{
			Sample poly("test-stereo.wav", 2);
			poly.play();
			poly.play();
			pump();
			Near(buffer[200], 0.5f, "two effect voices mix");
			poly.play();
			pump();
			Near(buffer[200], 0.5f, "polyphony capped at two");
			Sample moved(std::move(poly));
			pump();
			Near(buffer[200], 0.5f, "moved sample retains voices");
		}
		{
			Stream shortStream("test-short.wav");
			shortStream.play();
			pump();
			pump();
			pump();
			Check(!shortStream.isPlaying(), "stream end status");
			shortStream.play();
			pump();
			Near(buffer[200], 0.25f, "stream restarts at end");
		}
		{
			Stream loop("test-short.wav", 1, false, true, true);
			loop.play();
			for (int i = 0; i < 5; ++i)
				pump();
			Near(buffer[200], 0.25f, "looping stream");
		}
		{
			Stream speed("test-stereo.wav", 1, false, true, false, 1.5);
			speed.play();
			for (int i = 0; i < 50; ++i)
				pump();
			Check(speed.posSec().count() > 0, "tempo stream progresses");
			Check(speed.duration().count() > 0, "tempo stream duration");
		}
		{
			Stream decode("test-stereo.wav", 1, false, true, false, 1, true);
			pump();
			Near(buffer[200], 0, "explicit decode stream not auto mixed");
			std::vector<float> data(256);
			Check(decode.getData(data.data(), 1024) == 1024, "legacy decode remains accessible");
		}
		{
			const auto before = ksmaudio::GetAudioLatencySnapshot();
			Check(before.reportedValid && before.predictionValid, "ASIO latency values explicitly valid");
			Check(std::abs(before.predictedMaxSec - before.predictedMinSec - before.blockSec) < 1e-9, "prediction adds only one callback interval");
			Sample scheduled("test-stereo.wav", 1);
			Check(scheduled.schedulePlay(before.outputFrame + 64), "sample scheduled inside output block");
			pump();
			Near(buffer[0], 0, "scheduled sound silent before exact frame");
			Near(buffer[126], 0, "scheduled sound silent through frame 63");
			Near(buffer[128], 0.25f, "scheduled sound begins at frame 64");
			Near(buffer[129], 0.5f, "scheduled stereo starts together");
			scheduled.cancelScheduled();
		}
		pump();
		Near(buffer[200], 0, "destroyed scheduled owner detached");
		{
			Sample pending("test-short.wav");
			Check(pending.schedulePlay(ksmaudio::GetAudioLatencySnapshot().outputFrame + 64), "event queued before move");
			Sample moved(std::move(pending));
			pump();
			Near(buffer[200], 0, "move cancels events referencing old owner");
			moved.play();
			pump();
			Near(buffer[200], 0.25f, "moved scheduled sample can play normally");
		}
		{
			Sample cancelled("test-short.wav");
			const auto frame = ksmaudio::GetAudioLatencySnapshot().outputFrame;
			Check(cancelled.schedulePlay(frame + 64), "cancellable event queued");
			cancelled.cancelScheduled();
			pump();
			Near(buffer[200], 0, "cancelled event stays silent");
			for (unsigned i = 0; i < 4096; ++i)
				if (!cancelled.schedulePlay(frame + 100000))
					throw std::runtime_error("queue rejected before capacity");
			Check(true, "bounded queue accepts 4096 events");
			Check(!cancelled.schedulePlay(frame + 100000), "bounded queue rejects overflow");
			ksmaudio::CancelScheduledSounds();
			pump();
			Near(buffer[200], 0, "transport cancels pending events");
			const auto now = ksmaudio::GetAudioLatencySnapshot();
			Check(cancelled.schedulePlay(now.outputFrame - 1), "late event accepted once");
			pump();
			Check(ksmaudio::GetAudioLatencySnapshot().lateEvents > 0, "late event counted");
		}
		{
			Stream source("test-stereo.wav");
			source.seekPosSec(ksmaudio::SecondsF{ 1 });
			Near(static_cast<float>(source.posSec().count()), 1, "paused seek retains requested origin");
			source.play();
			const auto c0 = source.clockSnapshot();
			Check(c0.valid && c0.audiblePositionSec >= 1, "startup clock never falls below seek origin");
			pump();
			pump();
			pump();
			const auto c1 = source.clockSnapshot();
			const auto expected = ksmaudio::GetAudioOutputStatus().latencySec;
			Check(std::abs(c1.renderPositionSec - c1.audiblePositionSec - expected) < 0.001, "render and audible clocks differ by latency once");
			source.pause();
			const auto paused = source.posSec();
			pump();
			pump();
			Check(source.posSec() == paused, "paused audible clock freezes");
			Stream moved(std::move(source));
			moved.play();
			pump();
			Near(buffer[200], 0.25f, "stream move transfers sole handle ownership");
			Stream assigned("test-short.wav");
			assigned = std::move(moved);
			pump();
			Near(buffer[200], 0.25f, "stream move assignment retains source");
		}
		// Never emit test tones through the physical Windows output during switches.
		ksmaudio::SetMasterVolume(0);
		{
			struct Half final : ksmaudio::AudioEffect::IAudioEffect
			{
				void process(float* p, std::size_t n) override
				{
					for (std::size_t i = 0; i < n; ++i)
						p[i] *= 0.5f;
				}
				void updateStatusByFX(const ksmaudio::AudioEffect::Status&, std::optional<std::size_t>) override
				{
				}
				void updateStatusByLaser(const ksmaudio::AudioEffect::Status&, bool) override
				{
				}
				void setParamValueSet(ksmaudio::AudioEffect::ParamID, const ksmaudio::AudioEffect::ValueSet&) override
				{
				}
				std::unordered_map<ksmaudio::AudioEffect::ParamID, ksmaudio::AudioEffect::ValueSet> paramValueSetDict() const override
				{
					return {};
				}
				void setBypass(bool) override
				{
				}
			} half;
			Stream bgm("test-stereo.wav", 0.4, false, true, true);
			bgm.seekPosSec(ksmaudio::SecondsF{ 0.5 });
			bgm.play();
			Stream muted("test-stereo.wav", 0.8);
			muted.setMuted(true);
			muted.play();
			Sample cache("test-short.wav", 2);
			const auto token = bgm.addAudioEffect(&half, 10);
			const auto count = ksmaudio::detail::OutputResources().size();
			for (auto native : { ksmaudio::AudioBackend::Default, ksmaudio::AudioBackend::DirectSound })
			{
				for (int i = 0; i < 20; ++i)
				{
					std::vector<std::string> switchPhases;
					ksmaudio::AudioOutputRequest request{ native, {} };
					request.progress = [&](const char* phase)
					{
						switchPhases.push_back(phase);
					};
					const auto nativeResult = ksmaudio::ApplyOutputChange(nullptr, request);
					const auto stopCalls = reinterpret_cast<unsigned(WINAPI*)()>(GetProcAddress(GetModuleHandleW(L"bassasio.dll"), "FakeAsioStopCalls"));
					Check(stopCalls && stopCalls() == 1, "active ASIO driver stopped exactly once per transition");
					Check(!switchPhases.empty() && switchPhases.front() == "capture playback state" && switchPhases.back() == "complete", "output switch reports ordered progress through completion");
					Check(nativeResult.success && nativeResult.available, "native output switch succeeds");
					bool hasBegin = false, hasEnd = false, hasInfoBegin = false, hasInfoEnd = false;
					for (const auto& phase : switchPhases)
					{
						if (phase.starts_with("BEGIN BASS_Init("))
							hasBegin = true;
						if (phase.starts_with("END BASS_Init(") && phase.find("result=1 bass_error=0 elapsed_ms=") != std::string::npos)
							hasEnd = true;
						if (phase == "BEGIN BASS_GetInfo()")
							hasInfoBegin = true;
						if (phase.starts_with("END BASS_GetInfo() result=1"))
							hasInfoEnd = true;
					}
					Check(hasBegin && hasEnd && hasInfoBegin && hasInfoEnd, "native initialization logs call boundaries, results, errors and elapsed time");
					Check(BASS_GetConfig(BASS_CONFIG_BUFFER) == 200, "legacy stream buffer remains 200 ms");
					Check(BASS_GetConfig(BASS_CONFIG_UPDATEPERIOD) == 100, "legacy update remains 100 ms");
					Check(bgm.isPlaying() && muted.isPlaying(), "stream playback restored");
					Check(bgm.volume() == 0.4 && muted.muted(), "source volume and mute restored");
					const auto back = ksmaudio::ApplyOutputChange(nullptr, { ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO" });
					Check(back.success && back.available, "ASIO switch back succeeds");
					Check(ksmaudio::detail::OutputResources().size() == count, "resource registry stable across switches");
					ksmaudio::SetMasterVolume(1);
					pump();
					Near(buffer[200], 0.05f, "DSP and source volume rebound after switch");
					ksmaudio::SetMasterVolume(0);
				}
			}
			const auto fail = ksmaudio::ApplyOutputChange(nullptr, { ksmaudio::AudioBackend::ASIO, "Missing ASIO driver" });
			Check(!fail.success && fail.restoredPrevious && fail.available, "failed switch rolls back previous output");
			Check(bgm.isPlaying() && bgm.volume() == 0.4 && muted.muted(), "rollback restores streams and mute");
			Check(!fail.error.empty(), "failed switch reports reason");
			bgm.removeAudioEffect(token);
			ksmaudio::SetMasterVolume(1);
			pump();
			Near(buffer[200], 0.1f, "logical DSP token removes recreated native DSP");
			cache.play();
			pump();
			Near(buffer[200], 0.35f, "sample cache usable after repeated switches");
			ksmaudio::SetMute(true);
			const auto native = ksmaudio::ApplyOutputChange(nullptr, {});
			Check(native.success, "switch while globally muted");
			const auto back = ksmaudio::ApplyOutputChange(nullptr, { ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO" });
			Check(back.success, "muted ASIO restore");
			pump();
			Near(buffer[200], 0, "global mute survives output recreation");
			ksmaudio::SetMute(false);
		}
		ksmaudio::SetMasterVolume(0);
		{
			Stream shortEnd("test-short.wav");
			shortEnd.play();
			for (int i = 0; i < 20; ++i)
				pump();
			Check(std::abs(shortEnd.posSec().count() - shortEnd.duration().count()) < 0.0001, "audible clock reaches actual end after draining");
		}
		{
			WAV("test-restorable.wav");
			Stream restorable("test-restorable.wav", 0.3, false, true, true);
			restorable.seekPosSec(ksmaudio::SecondsF{ 0.6 });
			restorable.play();
			Check(std::rename("test-restorable.wav", "test-restorable.hidden") == 0, "simulate unavailable source file");
			const auto failed = ksmaudio::ApplyOutputChange(nullptr, { ksmaudio::AudioBackend::Default, {} });
			Check(!failed.available && !failed.success, "all reconstruction failures leave safe silent output");
			Check(!ksmaudio::GetAudioLatencySnapshot().available, "silent state explicitly reported");
			Check(std::rename("test-restorable.hidden", "test-restorable.wav") == 0, "restore unavailable source file");
			const auto retry = ksmaudio::ApplyOutputChange(nullptr, { ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO" });
			Check(retry.success && restorable.isPlaying(), "retry after silent failure rebuilds original live source");
			Check(restorable.volume() == 0.3 && restorable.posSec().count() > 0.59, "retry preserves source volume and transport");
		}
		ksmaudio::SetMasterVolume(1);
		Check(ksmaudio::detail::OutputResources().empty(), "all live resources unregistered after destruction");
		{
			Stream active("test-stereo.wav");
			active.play();
			ksmaudio::Terminate();
			Check(!active.isPlaying(), "shutdown invalidates surviving stream handles safely");
			Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "initialization after surviving resource shutdown");
			Sample replacement("test-short.wav");
			replacement.play();
			pump();
			Near(buffer[200], 0.25f, "old resource teardown cannot free newly reused handle");
		}
		Check(ksmaudio::GetAudioOutputFailure().empty(), "healthy driver status");
		notify(2);
		Check(!ksmaudio::GetAudioOutputFailure().empty(), "driver reset detected");
		ksmaudio::Terminate();
		Check(!ksmaudio::IsASIOActive(), "ASIO shutdown");
		Check(ksmaudio::Init(nullptr, ksmaudio::AudioBackend::ASIO, "KSM Mock ASIO"), "ASIO reinitialization");
		Check(ksmaudio::GetAudioOutputFailure().empty(), "reset state cleared");
		ksmaudio::Terminate();
		std::cout << "TOTAL " << checks << " checks passed (mock ASIO, real BASS/BASSmix)\n";
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cerr << "FAIL " << e.what() << ", BASS error=" << BASS_ErrorGetCode() << '\n';
		ksmaudio::Terminate();
		return 1;
	}
}
