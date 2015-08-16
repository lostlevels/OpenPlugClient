#pragma once

#include <vector>

struct Song {
	std::string name;
	std::string description;
	std::string url;
	int         duration;
	int         filesize;
	float       priority;
	int         id;

	Song(const std::string &name, const std::string &url, int duration, int filesize) : name(name), url(url), duration(duration), filesize(filesize) {}
	Song() : duration(0), filesize(0) {}
};