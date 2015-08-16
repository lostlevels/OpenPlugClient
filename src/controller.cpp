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

#define HEADING_PAIR 1
#define TEXT_PAIR 2
#define SELECTED_PAIR 3

Controller::Controller(const std::string &host) :
	host(host),
	messages(16),
	running(true),
	playing(false),
	waiting_for_file(false),
	fetching_playlist_songs(false),
	fetching_playlist_single(false),
	should_fetch_playlist_single(false),
	retrieving_song_info(false) {

	init_pair(HEADING_PAIR, COLOR_WHITE, COLOR_BLUE);
	init_pair(TEXT_PAIR, COLOR_BLACK, COLOR_WHITE);
	init_pair(SELECTED_PAIR, COLOR_CYAN, COLOR_WHITE);

	for (int i = 0; i < 10; i++) {
		std::string download_file = get_download_file_prefix() + std::to_string(i);
		remove(download_file.c_str());

		std::string log_file = get_log_file_prefix() + std::to_string(i);
		remove(log_file.c_str());
	}

	add_message("Welcome to OpenPlugServer.");
}

Controller::~Controller() {
	if (download_thread.joinable()) download_thread.join();

	for (auto &kv : cached_files) {
		remove(kv.second.c_str());
	}
}

void Controller::draw() {
	int max_y, max_x;
	getmaxyx(stdscr, max_y, max_x);
	clear();
	std::string blank_line = String::get_padded_string("", max_x, ' ');

	bkgd(COLOR_PAIR(0));

	attron(COLOR_PAIR(HEADING_PAIR));
	if (!timer.is_stopped() && !timer.is_done()) {
		mvprintw(max_y - 2, 0, blank_line.c_str());
		std::string song_heading = current_song.name + " Time: " + std::to_string(static_cast<int>(timer.get_time_remaining()));
		mvprintw(max_y - 2, 0, song_heading.c_str());
	}
	mvprintw(0, 0, blank_line.c_str());
	std::string heading = !playlist.name.empty() ? "In room " + playlist.name : "OpenPlugServer";
	mvprintw(0, 0, heading.c_str());
	attroff(COLOR_PAIR(HEADING_PAIR));

	attron(COLOR_PAIR(TEXT_PAIR));
	messages.draw(1, 0, max_y - 2, max_x);
	attroff(COLOR_PAIR(HEADING_PAIR));

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
}

void Controller::process_line(const std::string &line) {
	auto args = String::split_quoted(line, " ");
	if (args.size() < 1) return;
	auto &command = args[0];

	if (command == "playlist" && args.size() == 2) {
		process_command_playlist(args);
	}
	else if (command == "clear") {
		messages.clear();
	}
	else if (command == "songs") {
		for (auto i = 0; i < playlist.order.size(); i++) {
			auto &song_name = playlist.songs[playlist.order[i]].name;
			bool bold = song_name == current_song.name;
			if (bold) add_bold_message(song_name);
			else add_message(song_name);
		}
	}
	else if (command == "playlist" && args.size() == 3 && args[2] == "play") {
		process_command_playlist_single(args);
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

			const rapidjson::Value &songs_array = json["data"];
			for (auto i = 0; i < songs_array.Size(); i++) {
				add_message("%d. %s", i + 1, songs_array[i]["title"].GetString());
				int song_length = songs_array[i]["length"].GetInt();
				int id = songs_array[i]["id"].GetInt();
				Song song(songs_array[i]["title"].GetString(), songs_array[i]["youtube_url"].GetString(),
					song_length, songs_array[i]["filesize"].GetInt());
				song.priority = songs_array[i]["priority"].GetDouble();
				song.id = songs_array[i]["id"].GetInt();

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

void Controller::process_command_playlist_single(const Arguments &args, bool update_playlist_first) {
	if (args.size() < 3) return;
	if (playlist.songs.empty() || update_playlist_first) {
		should_fetch_playlist_single = true;
		process_command_playlist({args[0], args[1]});
		return;
	}

	playlist.name = args[1];
	fetching_playlist_single = true;
	should_fetch_playlist_single = false;

	requests.get(build_api("/playlists/" + playlist.name), [&](const std::string &response, bool success) -> void {
		rapidjson::Document json;
		fetching_playlist_single = false;

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

				if (time_left < 3) {
					// nearing end. call self again to avoid noise
					should_fetch_playlist_single = true;
					return;
				}

				add_message("Seeking %d seconds for time left of %d", elapsed_seconds, time_left);
				timer.reset(time_left);
				play_song(playlist.songs[id], elapsed_seconds);
			}
		}
	});
}

void Controller::process_command_playlist_add(const Arguments &args) {
	if (args.size() < 3) return;

	last_created_playlist = args[1];
	if (args.size() == 3 && args[2] == "add") {
		add_playlist(last_created_playlist);
	}
	else if (args.size() == 4 && args[2] == "add") {
		add_song(args[3], last_created_playlist);
	}
}

void Controller::add_playlist(const std::string &playlist_name) {
	requests.post(build_api("/playlists/?playlist_name=" + playlist_name), "", [&](const std::string &response, bool success) {
		rapidjson::Document json;

		if (json.Parse(response.c_str()).HasParseError()) {
			return add_message("Playlist error");
		}

		add_message(response);
		if (json.HasMember("status") && json["status"].GetBool()) {
			add_message("Created playlist " + playlist_name);
		}
		else {
			add_message("Unable to create playlist");
		}
	});
}

void Controller::add_song(const std::string &youtube_url, const std::string &to_playlist) {
	// If retrieved_song_info is set, we still need to send the previous one.
	if (retrieving_song_info || retrieved_song_info) add_message("waiting on another thread.");
	if (retrieving_song_info) return;
	if (add_song_thread.joinable()) add_song_thread.join();

	std::thread thread(&Controller::add_song_run, this, youtube_url, to_playlist);
	add_song_thread = std::move(thread);
}

void Controller::add_song_run(const std::string &youtube_url, const std::string &to_playlist) {
	retrieving_song_info = true;
	retrieved_song_info = false;

	// will block so in own thread.
	Song song = get_song_info(youtube_url, get_info_log_file());
	std::string url = build_api("/playlists/" + to_playlist + "/songs?" + build_post_parameters(song));
	std::lock_guard<std::mutex> guard(mutex);
	song_url_to_post = url;
	song_added = song.name;
	retrieving_song_info = false;
	retrieved_song_info = true;
}

void Controller::play_song(const Song &song, int time_offset_seconds) {
	player.stop_processing();
	playing = waiting_for_file = true;
	current_time_offset = time_offset_seconds;
	current_song = song;

	if (cached_files.find(song.url) == cached_files.end()) {
		if (download_thread.joinable()) download_thread.join();

		current_player_file = cached_files[song.url] = generate_new_download_file();
		std::thread thread(download_file, song.url, current_player_file, generate_new_log_file());
		download_thread = std::move(thread);
	}
	else {
		current_player_file = cached_files[song.url];
		log_text("playing cached song %s", current_player_file.c_str());
	}
}

bool Controller::finished_song() const {
	return !timer.is_stopped() && timer.is_done();
}

bool Controller::should_fetch_song() const {
	if (fetching_playlist_single || fetching_playlist_songs || playlist.songs.empty()) return false;
	return finished_song() || should_fetch_playlist_single;
}

void Controller::try_cache_next_song() {
	if (!playing || download_thread.joinable() || timer.get_time_remaining() < 60) return;

	int id = current_song.id;
	for (int i = 0; i < (int)playlist.order.size(); i++) {
		if (playlist.order[i] == id) {
			int next_id = i < playlist.order.size() - 1 ? playlist.order[i + 1] : playlist.order[0];
			if (playlist.songs.find(next_id) != playlist.songs.end() && cached_files.find(playlist.songs[next_id].url) == cached_files.end()) {
				Song next_song = playlist.songs[next_id];
				cached_files[next_song.url] = generate_new_download_file();
				add_message("caching next song to %s", cached_files[next_song.url].c_str());
				std::thread thread(download_file, next_song.url, cached_files[next_song.url], generate_new_log_file());
				download_thread = std::move(thread);
			}
			break;
		}
	}
}

void Controller::post_song() {
	if (!retrieved_song_info) return;

	// Should be done if retrieved_song_info set
	if (add_song_thread.joinable()) add_song_thread.join();

	retrieved_song_info = false;

	requests.post(song_url_to_post, "", [&](const std::string &response, bool success) {
		rapidjson::Document json;
		if (!json.Parse(response.c_str()).HasParseError()) {
			if (json.HasMember("id") && json.HasMember("priority")) {
				add_message("Added song " + song_added);
			}
			else {
				add_message("Missing fields " + response);
				log_text("song post result %s\n", response.c_str());
			}
		}
	});
}

void Controller::tick() {
	while (running) {
		process_input(getch());

		if (should_fetch_song()) {
			bool reload_playlist = finished_song();
			if (finished_song()) {
				timer.stop();
			}
			process_command_playlist_single({"playlist", playlist.name, "play"}, reload_playlist);
		}

		post_song();
		requests.tick();
		player_tick();
		// try_cache_next_song();
		draw();
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}

	player.stop_processing();
}

bool Controller::player_tick() {
	if (!playing) return false;

	if (waiting_for_file && access(current_player_file.c_str(), R_OK) == 0) {
		// need a few seconds worth plus more.
		int min_amount_needed = 65536;
		if (current_time_offset > 0) {
			min_amount_needed += static_cast<int>(current_song.filesize * current_time_offset/(float)current_song.duration);
		}
		min_amount_needed = std::min(min_amount_needed, current_song.filesize);

		if (File::get_filesize(current_player_file) >= min_amount_needed) {
			waiting_for_file = false;
			add_message("Playing " + current_song.name);
			bool result = player.load_file(current_player_file, current_time_offset);
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

void Controller::add_bold_message(const std::string &message) {
	messages.add_message(message, true);
	draw();
}
