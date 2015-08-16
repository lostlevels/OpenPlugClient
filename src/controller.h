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
	bool           fetching_playlist_single;
	bool           should_fetch_playlist_single;

	Player         player;
	bool           playing;
	Song           current_song;
	int            current_time_offset;
	std::string    current_player_file;
	bool           waiting_for_file;
	StopWatch      timer;

	bool           retrieving_song_info;
	bool           retrieved_song_info;

	std::thread    download_thread;
	bool           downloading;
	// std::string    downloading_url;

	std::thread    add_song_thread; // since would normally block when waiting for youtube-dl response
	std::mutex     mutex;
	std::string    song_url_to_post;
	std::string    song_added;

	void draw();

	void play_song(const Song &song, int time_offset_seconds = 0);
	bool player_tick();
	bool should_fetch_song() const;
	bool finished_song() const;
	void try_cache_next_song();
	void post_song();

	void process_input(int c);
	void process_line(const std::string &line);
	void play_current_song(const std::string &playlist_name, bool update_playlist_first = false);
	void fetch_playlist(const std::string &playlist_name, bool print_songs_after = false);
	void print_songs();

	// all 'run' functions are executed in own thread. Eerything else should be on main thread.
	void process_command_playlist_add(const Arguments &args);
	void add_song(const std::string &youtube_url, const std::string &to_playlist);
	void add_playlist(const std::string &playlist_name);
	void add_song_run(const std::string &youtube_url, const std::string &to_playlist);
	void download_file_run(const std::string &url, const std::string &output_file, const std::string &log_file);
	std::string build_api(const std::string &path);

	void add_message(const char *format, ...);
	void add_message(const std::string &message);
	void add_bold_message(const std::string &message);
};