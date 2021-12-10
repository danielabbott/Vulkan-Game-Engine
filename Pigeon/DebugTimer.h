#pragma once

#include <chrono>
#include <string>
#include <spdlog/spdlog.h>

class DebugTimer {
	std::string s;
	std::chrono::steady_clock::time_point start_time;
public:
	DebugTimer(std::string && s_) : s(move(s_)), start_time(std::chrono::steady_clock::now()) {}

	~DebugTimer() {
		using namespace std::chrono;
		steady_clock::time_point end_time = steady_clock::now();
		auto micro = duration_cast<microseconds>(end_time - start_time).count();
		if (micro < 1000) {
			spdlog::debug("{} took {}us", s, micro);
		}
		else {
			auto ms = micro / 1000.0f;
			spdlog::debug("{} took {}ms", s, ms);
		}
	}
};
