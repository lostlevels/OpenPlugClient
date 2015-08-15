#include "string.h"
#include <stdlib.h>

static bool is_inside(char needle, const std::string &haystack) {
	auto length = haystack.length();
	for (auto i = 0; i < length; i++) {
		if (haystack[i] == needle) return true;
	}
	return false;
}

std::vector<std::string> String::split(const std::string &string, const std::string &delimiters) {
	std::vector<std::string> words;

	int start = 0;
	int end = 0;

	for (end = 0; end < (int)string.length(); end++) {
		bool atDelimiter = is_inside(string[end], delimiters);
		if (atDelimiter || end == (int)string.length() - 1) {
			int wordLength = atDelimiter ? end - start : end + 1 - start;
			if (wordLength > 0) {
				words.push_back(string.substr(start, wordLength));
			}
			start = end + 1;
		}
	}

	return words;
}

std::vector<std::string> String::split_quoted(const std::string &string, const std::string &delimiters) {
	std::vector<std::string> words;

	int start = 0;
	int end = 0;
	int str_length = static_cast<int>(string.size());

	for (end = 0; end < str_length; end++) {
		if (string[end] == '"') {
			start = ++end;
			// Find the end
			while (string[end] != '"' && end < str_length) {
				end++;
			}
			if (end - start > 0) {
				words.push_back(string.substr(start, end - start));
			}
			// At end of loop end will be incremented so set start to what end will be.
			start = end + 1;
		}
		else {
			bool atDelimiter = is_inside(string[end], delimiters);
			if (atDelimiter || end == (int)string.length() - 1) {
				int wordLength = atDelimiter ? end - start : end + 1 - start;
				if (wordLength > 0) {
					words.push_back(string.substr(start, wordLength));
				}
				start = end + 1;
			}
		}
	}

	return words;
}

int String::get_port(const std::string &url) {
	int offset = 0;
	if (url.find("http://") != std::string::npos) offset = strlen("http://");
	
	auto pos = url.find(":", offset);
	if (pos != std::string::npos) {
		return atoi(url.substr(pos + 1).c_str());
	}
	return 80;
}

std::string String::get_route(const std::string &url) {
	std::size_t pos;
	std::size_t offset = 0;
	if (url.find("http://") == 0) {
		offset += strlen("http://");
	}

	pos = url.find("/", offset);
	return pos != std::string::npos ? url.substr(pos) : url;
}

std::string String::strip_port(const std::string &url) {
	auto pos = url.rfind(":");
	return url.substr(0, pos);
}

std::string String::get_host(const std::string &url) {
	std::size_t offset = 0;
	if (url.find("http://") != std::string::npos) {
		offset += strlen("http://");
	}
	return strip_port(url.substr(offset, url.find("/") - offset));
}


std::string String::url_encode(const std::string &url) {
	std::string copy = url;
	auto pos = copy.find(" ");
	while (pos != std::string::npos) {
		copy[pos] = '+';
		pos = copy.find(" ", pos + 1);
	}
	return copy;
}