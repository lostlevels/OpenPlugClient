#pragma once

#include "decoder.h"
#include "streamer.h"

//
// Client ties together decoding, streaming, parsing
//

class Client {
public:
	Client();
	~Client();

	void tick();
	bool load_file(const std::string &filename);
	bool is_file_loaded() const;

	bool is_file_done_playing() const;

private:
	bool        file_loaded;

	Decoder     decoder;
	Streamer    streamer;
};

inline bool Client::is_file_loaded() const {
	return file_loaded;
}

inline bool Client::is_file_done_playing() const {
	return decoder.is_done_decoding() && streamer.is_done_playing();
}