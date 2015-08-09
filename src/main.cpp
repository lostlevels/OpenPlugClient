#include "client.h"
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <curl/curl.h>

int main(int argc, const char * argv[]) {

	if (argc <= 1) {
		fprintf(stdout, "Usage: %s youtube_link\n", argv[0]);
		return 1;
	}

	// Init
	Pa_Initialize();
	curl_global_init(CURL_GLOBAL_ALL);
	av_register_all();

	// Download
	std::string temp_file = "/Users/jimmy/Desktop/download.temp";
	char command[1024] = {0};
	const char *youtube_link = argv[1];
	snprintf(command, sizeof(command) - 1, "/Users/jimmy/Desktop/youtube-dl -o '%s' -f 140 %s", temp_file.c_str(), youtube_link);
	system(command);

	Client client;
	bool result = client.load_file(temp_file);
	assert(result);

	// Run loop
	while (!client.is_file_done_playing()) {
		client.tick();
	}

	// Destroy
	curl_global_cleanup();
	Pa_Terminate();

	return 0;
}
