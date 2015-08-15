#include "file.h"
#include <stdio.h>
#include <stdlib.h>

int File::get_filesize_from_log(const std::string &filename) {
	FILE *file = fopen(filename.c_str(), "rb");
	if (!file) return 0;

	// determine size from line such as
	// "[K[download]   0.0% of 4.93MiB at 38.64KiB/s ETA 02:10"
	const char *prefix = "% of ";

	char buffer[8192] = {0};
	fread(buffer, 8192, 1, file);
	fclose(file);

	const char *find = strstr(buffer, prefix);
	if (!find) return 0;

	int i;

	find += strlen(prefix);
	for (i = 0; i < strlen(find) && find[i] != 'M'; i++){}

	if (i < strlen(find)) return static_cast<int>(atof(find) * 1024 * 1024);
	else return 0;
}

int File::get_filesize(const std::string &filename) {
	FILE *file = fopen(filename.c_str(), "rb");
	if (!file) return 0;

	fseek(file, 0, SEEK_END);
	auto size = ftell(file);
	fclose(file);

	return static_cast<int>(size);
}

std::string get_temp_folder() {
#ifdef _WIN32
	return "/Windows/Temp";
#else
	return "/tmp";
#endif
}

// Log file for when getting info of a song with youtube-dl
// This will include song name and duration
std::string get_info_log_file() {
	return get_temp_folder() + "/opc_info.log";
}

std::string get_download_file_prefix() {
	return get_temp_folder() + "/opc_download_file";
}

std::string generate_new_download_file() {
	static int i = 0;
	return get_download_file_prefix() + std::to_string(i++);
}

std::string get_log_file_prefix() {
	return get_temp_folder() + "/opc_log";
}

std::string generate_new_log_file() {
	static int i = 0;
	return get_log_file_prefix() + std::to_string(i++);
}