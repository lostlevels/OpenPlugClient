#pragma once

#include <vector>
#include <portaudio.h>

//
// Streamer feeds audio to system using portaudio
//

class Streamer {
public:
	Streamer();
	~Streamer();

	// Pass in data for streamer to copy periodically
	void feed_data(const std::vector<float> &samples);

	// Call anytime new format / channels / sample_rate
	void create_stream(int channels, PaSampleFormat format, double sample_rate);
	void destroy_stream();
	int on_stream_update(void *outputBuffer, unsigned long frames_per_buffer);

	int get_buffered_data_size() const;
	bool is_done_playing() const;

private:
	PaStream *stream;
	int       num_channels;

	int last_data_position;
	std::vector<float> data;

	void trim_data();
};

inline int Streamer::get_buffered_data_size() const {
	return static_cast<int>(data.size());
}

inline bool Streamer::is_done_playing() const {
	return get_buffered_data_size() == 0;
}