#include "player.h"

Player::Player() : file_loaded(false) {

}

Player::~Player() {
	streamer.destroy_stream();
}

void Player::stop_processing() {
	decoder.stop();
	streamer.stop();
}

bool Player::load_file(const std::string &filename, int time_offset_seconds) {
	file_loaded = false;
	if (decoder.load_file(filename)) {
		if (time_offset_seconds > 1) {
			decoder.seek_to(time_offset_seconds);
		}
		file_loaded = true;
		auto info = decoder.get_decoder_info();

		streamer.create_stream(info.channels, paFloat32, info.sample_rate);
		return true;
	}
	return false;
}

void Player::tick() {
	if (!file_loaded || !decoder.is_loaded()) return;

	auto info = decoder.get_decoder_info();
	if (streamer.get_size_not_uploaded() < info.sample_rate * info.channels * 2) {
		decoder.tick([&](std::vector<float> &decoded_samples) {
			streamer.feed_data(decoded_samples);
		});
	}

	if (decoder.is_done_decoding()) streamer.stop();
}
