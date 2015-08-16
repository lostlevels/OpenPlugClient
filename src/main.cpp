#include "player.h"
#include "request.h"
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <curses.h>
#include "controller.h"
#include "string.h"
#include "downloader.h"
#include "request.h"
#include "log.h"

void system_init();
void system_destroy();

int main(int argc, const char * argv[]) {
	system_init();

	Controller controller("crunchserv.duckdns.org:25222");
	controller.tick();

	system_destroy();

	return 0;
}

void system_init() {
	Pa_Initialize();
	av_register_all();

	initscr();
	timeout(0);

	log_init("/tmp/opc_log.txt");
}

void system_destroy() {
	log_destroy();

	endwin();

	Pa_Terminate();
}
