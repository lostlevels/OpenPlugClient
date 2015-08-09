#include "client.h"
#include "request.h"
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <curl/curl.h>
#include <chrono>
#include <thread>
#include <unistd.h>

void download_file(const std::string &youtube_link, const std::string &out_file);
void system_init();
void system_destroy();

int main(int argc, const char * argv[]) {
	if (argc <= 1) {
		fprintf(stdout, "Usage: %s youtube_link\n", argv[0]);
		return 1;
	}

	system_init();

	// Download audio in different thread so we don't have to wait for download to finish
	std::string temp_file = "/Users/jimmy/Desktop/opc_temp_file";
	std::thread download_thread(download_file, argv[1], temp_file);

	// Wait for some of file to download
	while (access(temp_file.c_str(), F_OK) != -1) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Load audio
	Client client;
	bool result = client.load_file(temp_file);
	assert(result);

	// Run loop
	while (!client.is_file_done_playing()) {
		client.tick();
	}

	download_thread.join();

	// Destroy
	system_destroy();

	return 0;
}

void system_init() {
	Pa_Initialize();
	curl_global_init(CURL_GLOBAL_ALL);
	av_register_all();
}

void system_destroy() {
	curl_global_cleanup();
	Pa_Terminate();
}

void download_file(const std::string &youtube_link, const std::string &out_file) {
	// Delete out_file in case already exists.
	remove(out_file.c_str());
	
	char command[1024] = {0};
	snprintf(command, sizeof(command) - 1, "/Users/jimmy/Desktop/youtube-dl --no-part -o '%s' -f 140 %s", out_file.c_str(), youtube_link.c_str());
	system(command);
}