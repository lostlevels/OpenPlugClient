#pragma once

#include "decoder.h"
#include "streamer.h"

//
// Player ties together decoding, streaming, parsing
//

class Player {
public:
	Player();
	~Player();

	void tick();
	// Pass in manual_filesize if only part of file is downloaded but we know fullsize.
	// Needed so that we pass correct filesize to libav for proper playing
	bool load_file(const std::string &filename, int manual_filesize, int time_offset_seconds);
	bool is_file_loaded() const;

	bool is_file_done_playing() const;
	void stop_processing();

private:
	bool        file_loaded;

	Decoder     decoder;
	Streamer    streamer;
};

inline bool Player::is_file_loaded() const {
	return file_loaded;
}

inline bool Player::is_file_done_playing() const {
	return decoder.is_done_decoding() && streamer.is_done_playing();
}