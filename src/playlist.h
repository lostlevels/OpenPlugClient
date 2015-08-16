#pragma once

#include <string>
#include <vector>
#include "song.h"
#include <unordered_map>

struct Playlist {
public:
	std::string           name;
	std::string           description;
	// id => song
	std::unordered_map<int, Song> songs;
	// order of ids
	std::vector<int>      order;
	int                   current_song;
};