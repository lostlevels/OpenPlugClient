#include "request.h"
#include "log.h"
#include "string.h"
#include <string.h>
#include <assert.h>

struct RequestUserData {
	Request    *request;
	std::string url;

	RequestUserData(Request *request, const std::string &url) : request(request), url(url) {

	}

private:
	RequestUserData(const RequestUserData&) = delete;
	RequestUserData& operator=(const RequestUserData&) = delete;
};

static void begin_callback(const happyhttp::Response* r, void* user_data) {

}

void data_callback(const happyhttp::Response* r, void* user_data, const unsigned char* data, int n) {
	RequestUserData *request_user_data = static_cast<RequestUserData*>(user_data);
	request_user_data->request->buffer_response(request_user_data->url, data, n);
}

void complete_callback(const happyhttp::Response* r, void* user_data) {
	RequestUserData *request_user_data = static_cast<RequestUserData*>(user_data);
	request_user_data->request->inform_callbacks(request_user_data->url);

	// Only the user data, not the request object itself.
	delete request_user_data;
}

Request::Request() {

}

Request::~Request() {

}

void Request::buffer_response(const std::string &url, const uint8_t *data, int size) {
	if (buffered_responses.find(url) == buffered_responses.end())
		buffered_responses[url] = "";

	static char buffer[128 * 1024] = {0};
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, data, std::min(size, (int)sizeof(buffer) - 1));
	buffered_responses[url] += buffer;
}

void Request::inform_callbacks(const std::string &url) {
	auto callback = callbacks.find(url) != callbacks.end() ? callbacks[url] : nullptr;
	callbacks.erase(url);

	auto response = (buffered_responses.find(url) != buffered_responses.end()) ? buffered_responses[url] : "";
	buffered_responses.erase(url);

	if (callback) callback(response, true);

	log_text("response for %s : %s\n\n\n", url.c_str(), response.c_str());
}

void Request::post(const std::string &url, const std::string &post_fields, RequestCallback callback) {
	if (url.empty()) return;
	callbacks[url] = callback;

	std::string encoded_url = String::url_encode(url);

	happyhttp::Connection *conn = new happyhttp::Connection(String::get_host(encoded_url).c_str(), String::get_port(encoded_url));
	connections.push_back(conn);

	RequestUserData *user_data = new RequestUserData(this, url);
	conn->setcallbacks(begin_callback, data_callback, complete_callback, user_data);
	auto route = String::get_route(encoded_url);
	auto host = String::get_host(encoded_url);
	conn->request("POST", String::get_route(encoded_url).c_str(), nullptr, (const unsigned char*)post_fields.c_str(), (int)post_fields.size());

	tick();
}


void Request::get(const std::string &url, RequestCallback callback) {
	if (url.empty()) return;
	callbacks[url] = callback;
	
	std::string encoded_url = String::url_encode(url);
	happyhttp::Connection *conn = new happyhttp::Connection(String::get_host(encoded_url).c_str(), String::get_port(encoded_url));
	connections.push_back(conn);

	RequestUserData *user_data = new RequestUserData(this, encoded_url);
	conn->setcallbacks(begin_callback, data_callback, complete_callback, user_data);
	conn->request("GET", String::get_route(encoded_url).c_str());

	tick();
}

void Request::tick() {
	for (auto i = 0; i < connections.size(); i++) {
		auto conn = connections[i];
		if (conn->outstanding()) {
			conn->pump();
		}
		else {
			connections.erase(connections.begin() + i);
			delete conn;
			i--;
		}
	}
}