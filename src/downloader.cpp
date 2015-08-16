#include "downloader.h"
#include "string.h"
#include "log.h"
#include <thread>
#include <stdlib.h>
#include <unistd.h>

void download_file(const std::string &youtube_link, const std::string &output_file, const std::string &log_file) {
	// 140 = aac
	// 171 = vorbis
	static char command[2048] = {0};
	snprintf(command, sizeof(command) - 1, "export PATH=$PATH:/usr/local/bin; youtube-dl --no-cache-dir --no-part -o '%s' -f 171 %s > %s", output_file.c_str(), youtube_link.c_str(), log_file.c_str());
	system(command);
	log_text("downloading file %s done to %s %s!\n", youtube_link.c_str(), output_file.c_str(), log_file.c_str());
}

void get_song_info(const std::string &youtube_link, const std::string &log_file, SongInfoCallback callback) {
	std::thread thread([&] {
		Song song_info = get_song_info(youtube_link, log_file);
		if (callback) callback(song_info, true);
	});
	thread.detach();
}

static char *fgets_trim(char *str, int num, FILE *stream) {
	auto result = fgets(str, num, stream);
	str[strcspn(str, "\r\n")] = '\0';
	return result;
}

Song get_song_info(const std::string &youtube_link, const std::string &log_file) {
	char command[1024] = {0};
	snprintf(command, sizeof(command) - 1, "export PATH=$PATH:/usr/local/bin; youtube-dl --no-cache-dir --get-title --get-duration %s > %s", youtube_link.c_str(), log_file.c_str());
	system(command);

	Song song_info("", "", 0, 0);
	FILE *file = fopen(log_file.c_str(), "rb");
	if (!file) return song_info;

	std::string name;
	float duration;
	char buffer[256];

	// First line will be title
	fgets_trim(buffer, sizeof(buffer) - 1, file);
	name = buffer;

	// 2nd line will be duration
	fgets_trim(buffer, sizeof(buffer) - 1, file);
	std::vector<std::string> duration_parts = String::split(buffer, ":");
	int seconds_multiplier = 1;
	for (int i = static_cast<int>(duration_parts.size()) - 1; i > -1; i--) {
		duration += atoi(duration_parts[i].c_str()) * seconds_multiplier;
		seconds_multiplier *= 60;
	}

	fclose(file);

	song_info.name = name;
	song_info.url = youtube_link;
	song_info.duration = duration;

	// Now get file_size
	snprintf(command, sizeof(command) - 1, "export PATH=$PATH:/usr/local/bin; youtube-dl --no-cache-dir -F %s > %s", youtube_link.c_str(), log_file.c_str());
	system(command);

	file = fopen(log_file.c_str(), "rb");
	if (!file) return song_info;

	// Read line by line until we get right format
	while (fgets_trim(buffer, sizeof(buffer) - 1, file)) {
		if (strstr(buffer, "171")) {
			auto words = String::split(buffer, " \r\n");
			// last should be file size.
			if (!words.empty()) {
				auto &last = words.back();
				if (last.find("KiB") != std::string::npos)
					song_info.filesize = static_cast<int>(atof(words.back().c_str()) * 1024);
				else
					song_info.filesize = static_cast<int>(atof(words.back().c_str()) * 1024 * 1024);
				break;
			}
		}
	}
	fclose(file);
	log_text("song %s is %d seconds long with filesize %d\n", name.c_str(), (int)duration, song_info.filesize);

	return song_info;
}


