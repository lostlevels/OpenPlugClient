#pragma once

#include "ext/happyhttp.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <string>

// First argument is response, 2nd argument is success or not
typedef std::function<void(const std::string &, bool)> RequestCallback;

class Request {
public:
	Request();
	~Request();

	void tick();
	void get(const std::string &url, RequestCallback callback = nullptr);
	void post(const std::string &url, const std::string &post_fields, RequestCallback callback = nullptr);

	void inform_callbacks(const std::string &url);
	void buffer_response(const std::string &url, const uint8_t *data, int size);

private:
	std::vector<happyhttp::Connection*> connections;
	std::unordered_map<std::string, RequestCallback> callbacks;
	std::unordered_map<std::string, std::string> buffered_responses;

	Request(const Request&) = delete;
	Request &operator =(const Request&) = delete;
};