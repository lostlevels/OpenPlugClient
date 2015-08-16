#pragma once

#include <string>
#include <vector>

struct Message {
	std::string text;
	bool bold;
	
	Message(const std::string &text, bool bold) : text(text), bold(bold) {}
};

class MessageBuffer {
public:
	MessageBuffer(int max_entries);
	void draw(int y, int x, int max_y, int pad_x);
	void add_message(const std::string &message, bool bold = false);
	void clear();

private:
	int max_entries;
	std::vector<Message> messages;
};

inline void MessageBuffer::clear() {
	messages.clear();
}