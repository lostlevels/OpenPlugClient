#include "controller.h"
#include "file.h"
#include "string.h"
#include "downloader.h"
#include "log.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <assert.h>
#include <curses.h>

#define kEscapeKey 27
#define kDeleteKey 127

// Temp for now to avoid static ...
static void adjust_song_filesize(Song &song) {
	song.filesize = std::max(1, song.filesize - static_cast<int>(song.filesize / static_cast<float>(song.duration) * 2.5f));
}

Controller::Controller(const std::string &host) :
	host(host),
	messages(16),
	running(true),
	playing(false),
	waiting_for_file(false),
	fetching_playlist_songs(false),
	fetching_playlist_current(false),
	should_fetch_playlist_song(false) {

	for (int i = 0; i < 10; i++) {
		std::string download_file = get_download_file_prefix() + std::to_string(i);
		remove(download_file.c_str());

		std::string log_file = get_log_file_prefix() + std::to_string(i);
		remove(log_file.c_str());
	}

	add_message("Welcome to OpenPlugServer.");
}

Controller::~Controller() {
	if (download_thread_main.joinable()) download_thread_main.join();
	if (download_thread_second.joinable()) download_thread_second.join();

	for (auto &kv : cached_files) {
		remove(kv.second.c_str());
	}
}

void Controller::draw() {
	int max_y, max_x;
	getmaxyx(stdscr, max_y, max_x);
	clear();

	mvprintw(1, 0, "wait %d play %d loaded %d done %d", waiting_for_file, playing, player.is_file_loaded(), player.is_file_done_playing());

	if (!timer.is_stopped() && !timer.is_done()) mvprintw(0, 0, "Time: %02d", static_cast<int>(timer.get_time_remaining()));

	messages.draw(2, 0, max_y - 3);
	mvprintw(max_y - 1, 0, current_line.c_str());

	refresh();
}

void Controller::process_input(int c) {
	if (c == ERR) return;

	if (c == kEscapeKey || c == '\t') {
		running = false;
	}
	else if (c == kDeleteKey) {
		if (!current_line.empty()) current_line.erase(current_line.end() - 1);
	}
	else if (c == '\r' || c == '\n') {
		if (!current_line.empty()) {
			add_message("'" + current_line + "'");
			process_line(current_line);
		}

		current_line = "";
	}
	else {
		current_line += c;
	}

	draw();
}

void Controller::process_line(const std::string &line) {
	auto args = String::split_quoted(line, " ");
	if (args.size() < 1) return;
	auto &command = args[0];

	if (command == "playlist" && args.size() == 2) {
		process_command_playlist(args);
	}
	else if (command == "playlist" && args.size() == 3 && args[2] == "play") {
		process_command_playlist_current(args);
	}
	else if (command == "playlist") {
		process_command_playlist_add(args);
	}
}

std::string Controller::build_api(const std::string &path) {
	return host + path;
}

static std::string build_post_parameters(const Song &song) {
	std::string params = "";
	params += "youtube_url=" + song.url;
	params += "&title=" + song.name;
	params += "&artist=unknown";
	params += "&length=" + std::to_string(song.duration);
	params += "&filesize=" + std::to_string(song.filesize);
	return params;
}

void Controller::process_command_playlist(const Arguments &args) {
	if (args.size() < 2) return;
	fetching_playlist_songs = true;

	playlist.name = args[1];

	requests.get(build_api("/playlists/" + playlist.name + "/songs/"),  [&](const std::string &response, bool success) {
		fetching_playlist_songs = false;

		rapidjson::Document json;
		if (json.Parse(response.c_str()).HasParseError()) return;
		if (json.HasMember("total") && json.HasMember("data") && json["data"].IsArray()) {
			add_message("retrieved playlist " + playlist.name);
			playlist.songs.clear();
			playlist.order.clear();

			const rapidjson::Value& songs_array = json["data"];
			for (auto i = 0; i < songs_array.Size(); i++) {
				add_message("%d. %s", i + 1, songs_array[i]["title"].GetString());

				int song_length = songs_array[i]["length"].GetInt();
				int id = songs_array[i]["id"].GetInt();
				Song song(songs_array[i]["title"].GetString(), songs_array[i]["youtube_url"].GetString(),
					song_length, songs_array[i]["filesize"].GetInt());
				song.priority = songs_array[i]["priority"].GetDouble();

				adjust_song_filesize(song);
				playlist.songs[id] = song;
			}

			for (auto &kv : playlist.songs) {
				playlist.order.push_back(kv.first);
			}

			std::sort(playlist.order.begin(), playlist.order.end(), [&](const int a, const int b) -> bool {
				return playlist.songs[a].priority < playlist.songs[b].priority;
			});
		}
	});
}

void Controller::process_command_playlist_current(const Arguments &args) {
	if (args.size() < 3) return;
	if (playlist.songs.empty()) {
		// this will trigger player after getting playlist
		should_fetch_playlist_song = true;
		process_command_playlist({args[0], args[1]});
		return;
	}

	playlist.name = args[1];
	fetching_playlist_current = true;
	should_fetch_playlist_song = false;

	requests.get(build_api("/playlists/" + playlist.name), [&](const std::string &response, bool success) -> void {
		rapidjson::Document json;
		fetching_playlist_current = false;

		if (json.Parse(response.c_str()).HasParseError()) {
			log_text("json error!");
			return;
		}

		if (json.HasMember("current_song") && json.HasMember("song_start_time") && json.HasMember("requested_time")) {
			int id = json["current_song"].GetInt();

			if (playlist.songs.find(id) == playlist.songs.end()) {
				add_message("Cant find song in playlist - recache?");
			}
			else {
				int requested_time = json["requested_time"].GetInt();
				int song_start_time = json["song_start_time"].GetInt();

				int elapsed_seconds = requested_time - song_start_time;
				int time_left = playlist.songs[id].duration - elapsed_seconds;

				if (time_left < 5) {
					// nearing end. call self again to avoid noise
					should_fetch_playlist_song = true;
					return;
				}

				add_message("Seeking %d seconds for time left of %d", elapsed_seconds, time_left);
				timer.reset(time_left);
				play_song(playlist.songs[id], elapsed_seconds);

				// Also start downloading next song - todo make sure users connection can handle the download
				for (int i = 0; i < (int)playlist.order.size(); i++) {
					if (playlist.order[i] == id) {
						if (download_thread_second.joinable()) download_thread_second.join();

						int next_id = i < playlist.order.size() - 1 ? playlist.order[i + 1] : playlist.order[0];
						if (playlist.songs.find(next_id) != playlist.songs.end() && cached_files.find(playlist.songs[next_id].url) == cached_files.end()) {
							Song next_song = playlist.songs[next_id];
							cached_files[next_song.url] = generate_new_download_file();
							log_text("caching next song to %s", cached_files[next_song.url].c_str());
							std::thread download_thread(download_file, next_song.url, cached_files[next_song.url], generate_new_log_file());
							download_thread_second = std::move(download_thread);
						}
						break;
					}
				}
			}
		}
	});
}

void Controller::process_command_playlist_add(const Arguments &args) {
	if (args.size() < 3) return;

	last_created_playlist = args[1];
	if (args.size() == 3 && args[2] == "add") {
		// add playlist
		requests.post(build_api("/playlists/?playlist_name=" + last_created_playlist), "", [&](const std::string &response, bool success) {
			rapidjson::Document json;

			if (json.Parse(response.c_str()).HasParseError()) {
				add_message("Playlist error");
				return;
			}

			add_message(response);
			if (json.HasMember("status") && json["status"].GetBool()) {
				add_message("Created playlist " + last_created_playlist);
			}
			else {
				add_message("Unable to create playlist");
			}
		});
	}
	else if (args.size() == 4 && args[2] == "add") {
		// add song.
		auto youtube_url = args[3];
		Song song = get_song_info(youtube_url, get_info_log_file());

		std::string url = build_api("/playlists/" + last_created_playlist + "/songs?" + build_post_parameters(song));
		requests.post(url, "", [&](const std::string &response, bool success) {
			rapidjson::Document json;
			if (json.Parse(response.c_str()).HasParseError()) return;
			if (json.HasMember("id") && json.HasMember("priority")) {
				add_message("Added song " + song.name + " to " + last_created_playlist);
			}
			else {
				add_message("Missing fields " + response);
				log_text("song post result %s\n", response.c_str());
			}
		});
	}
}

void Controller::play_song(const Song &song, int time_offset_seconds) {
	player.stop_processing();
	playing = waiting_for_file = true;
	current_time_offset = time_offset_seconds;
	current_song = song;

	if (cached_files.find(song.url) == cached_files.end()) {
		if (download_thread_main.joinable()) download_thread_main.join();

		std::string downloaded_filename = cached_files[song.url] = generate_new_download_file();
		std::string log_file = generate_new_log_file();
		std::thread download_thread(download_file, song.url, downloaded_filename, log_file);
		download_thread_main = std::move(download_thread);

		log_text("downloading %s\n", downloaded_filename.c_str());
		log_text("with log %s\n", log_file.c_str());

		current_player_file = downloaded_filename;
	}
	else {
		current_player_file = cached_files[song.url];
		log_text("cached song %s", current_player_file.c_str());
	}
}

void Controller::tick() {
	while (running) {
		process_input(getch());

		if (!playlist.songs.empty() && (should_fetch_playlist_song || (!timer.is_stopped() && timer.is_done())) && !fetching_playlist_current) {
			add_message("playing next song %d %d", timer.is_done(), timer.is_stopped());
			timer.stop();
			process_command_playlist_current({"playlist", playlist.name, "play"});
		}

		requests.tick();
		player_tick();
		draw();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	player.stop_processing();
}

bool Controller::player_tick() {
	if (!playing) return false;

	if (waiting_for_file && access(current_player_file.c_str(), R_OK) == 0) {
		// hacky method to make sure enough is in
		int min_amount_needed = static_cast<int>(.25f * current_song.filesize);
		if (current_song.filesize < 1024 * 1024) min_amount_needed = 1024 * 1024;

		if (current_time_offset > 0) {
			min_amount_needed += static_cast<int>(current_song.filesize * current_time_offset/(float)current_song.duration);
		}
		min_amount_needed = std::min(min_amount_needed, current_song.filesize - 1024);

		if (File::get_filesize(current_player_file) >= min_amount_needed) {
			log_text("min_size is %d", min_amount_needed);
			waiting_for_file = false;
			add_message("Playing " + current_song.name);
			bool result = player.load_file(current_player_file, current_song.filesize, current_time_offset);
			assert(result);
		}
	}

	// Note still have to check for waiting_for_file
	// since last player.load_file can have both file loaded and done playing
	if (!waiting_for_file && player.is_file_loaded() && !player.is_file_done_playing()) {
		player.tick();
	}
	else if (!waiting_for_file && player.is_file_loaded() && player.is_file_done_playing()) {
		playing = false;
		waiting_for_file = false;
		return true;
	}

	return false;
}

void Controller::add_message(const char *format, ...) {
	static char buffer[4096];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(buffer, sizeof(buffer) - 1, format, argptr);
    va_end(argptr);

	messages.add_message(buffer);
	draw();
}

void Controller::add_message(const std::string &message) {
	messages.add_message(message);
	draw();
}
