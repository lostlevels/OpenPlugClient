#include "streamer.h"
#include "log.h"
#include <assert.h>

static int stream_callback(const void*, void *outputBuffer, unsigned long frames_per_buffer, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void *userData) {
	Streamer *streamer = static_cast<Streamer*>(userData);
	return streamer->on_stream_update(outputBuffer, frames_per_buffer);
}

void Streamer::stop() {
	destroy_stream();
}

void Streamer::create_stream(int channels, PaSampleFormat format, double sample_rate) {
	destroy_stream();

	num_misses = 0;
	num_channels = channels;
	auto result = Pa_OpenDefaultStream(&stream, 0, channels, format, sample_rate, 0, stream_callback, this);
	assert(result == paNoError);

	result = Pa_StartStream(stream);
	assert(result == paNoError);
}

void Streamer::feed_data(const std::vector<float> &samples) {
	if (samples.empty() || !stream) return;

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
	auto limit = frames_per_buffer * num_channels;
	auto actual_amount = data.size() - last_data_position >= limit ? limit : data.size() - last_data_position;
	float *out = (float*)outputBuffer;

	bool not_enough_data = data.size() - last_data_position < limit;
	int not_enough_amount = limit - (data.size() - last_data_position);

	memcpy(out, &data[last_data_position], actual_amount * sizeof(float));
	last_data_position += actual_amount;

	if (actual_amount < limit && actual_amount > 0) {
		for (auto i = actual_amount; i < limit; i++) {
			out[i] = 0;
		}
	}

	if (not_enough_data) {
		log_text("Not enough data by %d\n", not_enough_amount);
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

	data.clear();
	last_data_position = 0;
}
