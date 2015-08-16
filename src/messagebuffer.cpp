#include "messagebuffer.h"
#include "string.h"
#include <curses.h>

MessageBuffer::MessageBuffer(int max_entries) : max_entries(max_entries) {

}

static void pad_string(std::string &str, int num_chars, char with) {
	if (str.size() < num_chars) {
		str.insert(str.end(), num_chars - str.size(), with);
	}
}

void MessageBuffer::draw(int y, int x, int max_y, int pad_x) {
	for (int i = 0; i < std::min(max_y - y + 1, (int)messages.size()); i++) {
		auto &message = messages[i];
		std::string text = message.text;
		if (pad_x > 0) text = String::get_padded_string(text, pad_x, ' ');
		if (messages[i].bold) attron(A_BOLD);
		mvprintw(y + i, 0, text.c_str());
		attroff(A_BOLD);
	}
}

void MessageBuffer::add_message(const std::string &message, bool bold) {
	if (message.empty()) return;

	messages.push_back(Message(message, bold));
	if (messages.size() > max_entries) {
		int size = static_cast<int>(messages.size());
		int max = max_entries;
		int num_to_remove = size - max;
		if (num_to_remove < messages.size()) messages.erase(messages.begin(), messages.begin() + num_to_remove);
	}
}