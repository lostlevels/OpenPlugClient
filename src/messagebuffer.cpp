#include "messagebuffer.h"
#include <curses.h>

MessageBuffer::MessageBuffer(int max_entries) : max_entries(max_entries) {

}

void MessageBuffer::draw(int y, int x, int max_y) {
	for (int i = 0; i < std::min(max_y - y + 1, (int)messages.size()); i++) {
		mvprintw(y + i, 0, messages[i].c_str());
	}
}

void MessageBuffer::add_message(const std::string &message) {
	if (message.empty()) return;

	messages.push_back(message);
	if (messages.size() > max_entries) {
		int size = static_cast<int>(messages.size());
		int max = max_entries;
		int num_to_remove = size - max;
		if (num_to_remove < messages.size()) messages.erase(messages.begin(), messages.begin() + num_to_remove);
	}
}