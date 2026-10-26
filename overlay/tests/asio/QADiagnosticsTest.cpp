#include "ksmaudio/Diagnostics.hpp"
#include <iostream>
#include <vector>
int main(int argc, char** argv)
{
	using namespace ksmaudio::qa;
	if (argc < 3)
		return 2;
	const std::filesystem::path root{ argv[2] };
	if (std::string_view{ argv[1] } == "--disabled")
	{
		Event("DISABLED");
		Scope ignored{ "DISABLED", true };
		Phase("DISABLED");
		if (allocated != 0 || std::filesystem::exists(root))
			return 3;
		std::cout << "PASS disabled mode has no QA files/producers\n";
		return 0;
	}
	if (!Start(root))
		return 4;
	Event("TEXT", 12, 34, "hello\nuntrusted\ttext");
	std::vector<std::thread> producers;
	for (int i = 0; i < 8; ++i)
		producers.emplace_back([i]
							   {
								   Role("test-producer");
								   for (unsigned n = 0; n < 6000; ++n)
									   Event("BURST", i, n);
							   });
	for (auto& p : producers)
		p.join();
	std::atomic<bool> run{ true };
	std::thread audio([&]
					  {
						  Role("test-audio");
						  while (run)
						  {
							  Phase("test.audio.alive");
							  ++callbacks;
							  audioFrames += 128;
							  std::this_thread::sleep_for(std::chrono::milliseconds(2));
						  }
					  });
	{
		Scope stall{ "TEST.waiting_on_teardown", true, 99 };
		if (std::string_view{ argv[1] } == "--hang")
		{
			std::ofstream{ sessionPath / "hang-ready.txt" } << "ready\n";
			while (true)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(6200));
	}
	run = false;
	audio.join();
	Event("NORMAL_END");
	Stop();
	std::string text;
	for (const auto& file : std::filesystem::directory_iterator(sessionPath))
		if (file.path().extension() == ".log")
		{
			std::ifstream in{ file.path() };
			text.append(std::istreambuf_iterator<char>{ in }, {});
		}
	auto check = [&](bool yes, const char* label)
	{
		if (!yes)
		{
			std::cerr << "FAIL " << label << "\n";
			std::exit(5);
		}
		std::cout << "PASS " << label << "\n";
	};
	check(!writerFailed, "writer completed");
	check(text.find("STALL_HINT") != std::string::npos, "independent writer reports stalled main");
	check(text.find("phase=TEST.waiting_on_teardown") != std::string::npos, "exact pending scope preserved");
	check(text.find("test.audio.alive") != std::string::npos, "independent audio progress observed");
	check(text.find("SESSION_STOP") != std::string::npos, "normal shutdown flushed");
	check(text.find("NORMAL_END") != std::string::npos, "producer recovered after wait");
	check(text.find("LOST") != std::string::npos, "bounded-ring overflow reported");
	check(text.find("hello untrusted text") != std::string::npos, "text cannot inject log lines");
}
