#include "client.h"

Client::Client() : file_loaded(false) {

}

Client::~Client() {
	streamer.destroy_stream();
}

bool Client::load_file(const std::string &filename) {
	file_loaded = false;
	if (decoder.load_file(filename)) {
		file_loaded = true;
		auto info = decoder.get_decoder_info();

		streamer.create_stream(info.channels, paFloat32, info.sample_rate);
		return true;
	}

	return false;
}

void Client::tick() {
	if (!file_loaded || !decoder.is_loaded()) return;

	// If buffer has plenty of space just wait a bit.
	if (streamer.get_buffered_data_size() > 20 * 1024 * 1024) return;

	decoder.tick([&](std::vector<float> &decoded_samples) {
		streamer.feed_data(decoded_samples);
	});
}
