#pragma once

#include <string>
#include <vector>

class String {
public:
	static std::vector<std::string> split(const std::string &string, const std::string &delimiters);
	// things in quotes as a word
	static std::vector<std::string> split_quoted(const std::string &string, const std::string &delimiters);

	static int get_port(const std::string &url);
	static std::string get_route(const std::string &url);
	static std::string strip_port(const std::string &url);
	static std::string get_host(const std::string &url);

	static std::string url_encode(const std::string &url);
};
