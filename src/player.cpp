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
		recently_loaded = true;
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
	int ticks = recently_loaded ? 100 : 1;
	int min_samples = recently_loaded ? info.sample_rate * info.channels * 2 : info.sample_rate * info.channels * 5;

	while (ticks-- > 0) {
		if (streamer.get_size_not_uploaded() < min_samples) {
			decoder.tick([&](std::vector<float> &decoded_samples) {
				streamer.feed_data(decoded_samples);
			});
		}
		else {
			break;
		}
	}

	if (ticks > 0 && recently_loaded) {
		recently_loaded = false;
	}

	if (decoder.is_done_decoding()) streamer.stop();
}
