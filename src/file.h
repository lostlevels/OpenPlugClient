#pragma once

#include <string>

class File {
public:
	// Gets filesize from youtube-dl log file, parsing based on how it outputs
	static int get_filesize_from_log(const std::string &filename);

	// Get filesize with file functions
	static int get_filesize(const std::string &filename);
};

// Log file for when getting info of a song with youtube-dl
// This will include song name and duration
std::string get_info_log_file();
std::string get_download_file_prefix();
std::string generate_new_download_file();
std::string get_log_file_prefix();
std::string generate_new_log_file();
