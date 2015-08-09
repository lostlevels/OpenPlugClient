#include "request.h"
#include <string.h>
#include <assert.h>

static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
	static char buffer[16384] = {0};
	assert(sizeof(buffer) > size * nmemb);
	
	memcpy(buffer, ptr, size * nmemb);
	buffer[size * nmemb] = '\0';
	fprintf(stdout, "Received %s\n", buffer);

	return size * nmemb;
}

Request::Request() : num_handles(0) {
	multi_handle = curl_multi_init();
}

Request::~Request() {
	curl_multi_cleanup(multi_handle);
}

void Request::get(const std::string &url, std::function<void(const std::string &, bool)> callback) {
	CURL* curl = curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

	curl_multi_add_handle(multi_handle, curl);
}

void Request::tick() {
	int prev = num_handles;
	curl_multi_perform(multi_handle, &num_handles);
	if (!num_handles && prev) {
		
		struct CURLMsg *m = curl_multi_info_read(multi_handle, &num_handles);
		if (m) {
			printf("%d %d %d\n", m->msg, CURLMSG_DONE, m->data.result);
		}
	}
}