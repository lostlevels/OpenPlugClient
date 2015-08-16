#pragma once

#include <chrono>
typedef std::chrono::time_point<std::chrono::system_clock> Time;

class Timer {
public:
	Timer();

	void   reset();
	double get_time_elapsed() const;
private:
	Time start;
};

class StopWatch {
public:
	StopWatch();

	void   reset(double duration);
	double get_time_remaining() const;
	bool   is_done() const;
	bool   is_stopped() const;
	void   stop();
private:
	Time start;
	bool stopped;
	std::chrono::duration<double> duration;
};
