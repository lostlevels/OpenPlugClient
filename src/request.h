#pragma once

#include <curl/curl.h>
#include <string>
#include <functional>

class Request {
public:
	Request();
	~Request();

	void tick();
	void get(const std::string &url, std::function<void(const std::string &, bool)> = NULL);

private:
	CURLM    *multi_handle;
	int       num_handles;

	Request(const Request&) = delete;
	Request &operator =(const Request&) = delete;
};