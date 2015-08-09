#include "streamer.h"
#include <assert.h>

static int stream_callback(const void*, void *outputBuffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void *userData) {
	Streamer *streamer = static_cast<Streamer*>(userData);
	return streamer->on_stream_update(outputBuffer, frames_per_buffer);
}

void Streamer::create_stream(int channels, PaSampleFormat format, double sample_rate) {
	destroy_stream();

	num_channels = channels;
	auto result = Pa_OpenDefaultStream(&stream, 0, channels, format, sample_rate, 0, stream_callback, this);
	assert(result == paNoError);

	result = Pa_StartStream(stream);
	assert(result == paNoError);
}

void Streamer::feed_data(const std::vector<float> &samples) {
	if (samples.empty()) return;

	data.insert(data.end(), samples.begin(), samples.end());
	trim_data();
}

void Streamer::trim_data() {
	const int kSizeLimit = 32768;
	while (last_data_position > kSizeLimit) {
		data.erase(data.begin(), data.begin() + kSizeLimit);
		last_data_position -= kSizeLimit;
	}
}

int Streamer::on_stream_update(void *outputBuffer, unsigned long frames_per_buffer) {
	assert(data.size() - last_data_position >= frames_per_buffer * num_channels);

	float *out = (float*)outputBuffer;
	for(int i = 0; i < frames_per_buffer; i++) {
		for (int c = 0; c < num_channels; c++) {
			*out++ = data[last_data_position++];
			// *out++ = (rand()%100)/100.0f;
		}
	}

	return 0;
}

Streamer::Streamer() : stream(NULL), last_data_position(0) {
	data.reserve(16384);
}

Streamer::~Streamer() {
	destroy_stream();
}

void Streamer::destroy_stream() {
	if (stream) {
		Pa_AbortStream(stream);
		stream = NULL;
	}
}
