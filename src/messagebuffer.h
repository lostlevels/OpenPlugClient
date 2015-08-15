#pragma once

#include <string>
#include <vector>

class MessageBuffer {
public:
	MessageBuffer(int max_entries);
	void draw(int y, int x, int max_y);
	void add_message(const std::string &message);

private:
	int max_entries;
	std::vector<std::string> messages;
};
