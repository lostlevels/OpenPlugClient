#pragma once

#include <stdarg.h>

// Mainly since ncurses takes up stdout
void log_init(const char *file_out);
void log_text(const char *format, ...);
void log_destroy();