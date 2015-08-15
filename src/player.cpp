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

bool Player::load_file(const std::string &filename, int manual_filesize, int time_offset_seconds) {
	file_loaded = false;
	if (decoder.load_file(filename, manual_filesize)) {
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

	// If buffer has plenty of space just wait a bit - causing problems?????
	if (streamer.get_buffered_data_size() > 20 * 1024 * 1024) return;

	decoder.tick([&](std::vector<float> &decoded_samples) {
		streamer.feed_data(decoded_samples);
	});
}
