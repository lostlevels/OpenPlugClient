#include "log.h"
#include <stdio.h>

static FILE *file = nullptr;

void log_init(const char *file_out) {
	file = fopen(file_out, "w");
}

void log_text(const char *format, ...) {
	if (file) {
		va_list args;
		va_start(args, format);
		vfprintf(file, format, args);
		va_end (args);
	}
}

void log_destroy() {
	if (file) {
		file = nullptr;
		fclose(file);
	}
}