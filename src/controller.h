#pragma once

#include "player.h"
#include "timer.h"
#include "playlist.h"
#include "request.h"
#include "messagebuffer.h"

#include <thread>
#include <unistd.h>
#include <unordered_map>

typedef std::vector<std::string> Arguments;

class Controller {
public:
	Controller(const std::string &host = "");
	~Controller();
	void tick();

private:
	std::string    host;
	Request        requests;

	bool           running;

	// youtube_url => filename downloaded
	std::unordered_map<std::string, std::string> cached_files;
	Playlist       playlist;
	std::string    last_created_playlist; // for the add command
	std::string    current_line;
	MessageBuffer  messages;

	bool           fetching_playlist_songs;
	bool           fetching_playlist_current;
	bool           should_fetch_playlist_song;

	Player         player;
	bool           playing;
	Song           current_song;
	int            current_time_offset;
	std::string    current_player_file;
	bool           waiting_for_file;
	StopWatch      timer;

	std::thread    download_thread_main;
	std::thread    download_thread_second;

	void draw();

	void play_song(const Song &song, int time_offset_seconds = 0);
	bool player_tick();

	void process_input(int c);
	void process_line(const std::string &line);
	void process_command_playlist(const Arguments &args);
	void process_command_playlist_current(const Arguments &args);
	void process_command_playlist_add(const Arguments &args);
	std::string build_api(const std::string &path);

	void add_message(const char *format, ...);
	void add_message(const std::string &message);
};