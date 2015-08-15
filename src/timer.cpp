#include "timer.h"

Timer::Timer() {
	reset();
}

void Timer::reset() {
	start = std::chrono::system_clock::now();
}

double Timer::get_time_elapsed() const {
	std::chrono::duration<double> seconds = std::chrono::system_clock::now() - start;
	return seconds.count();
}

StopWatch::StopWatch() : stopped(true) {
	start = std::chrono::system_clock::now();
}

void StopWatch::reset(double duration) {
	stopped = false;
	this->duration = std::chrono::duration<double>(duration);
	start = std::chrono::system_clock::now();
}

double StopWatch::get_time_remaining() const {
	std::chrono::duration<double> remaining_seconds = duration - (std::chrono::system_clock::now() - start);
	return remaining_seconds.count();
}

bool StopWatch::is_done() const {
	return get_time_remaining() <= 0 || is_stopped();
}

bool StopWatch::is_stopped() const {
	return stopped;
}

void StopWatch::stop() {
	stopped = true;
}
