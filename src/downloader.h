#pragma once

#include <string>
#include <functional>
#include "song.h"

typedef std::function<void(const Song&, bool)> SongInfoCallback;

// blocking version
Song get_song_info(const std::string &youtube_link, const std::string &log_file);
// nonblocking version
void get_song_info(const std::string &youtube_link, const std::string &log_file, SongInfoCallback callback);

void download_file(const std::string &youtube_link, const std::string &output_file, const std::string &log_file);
