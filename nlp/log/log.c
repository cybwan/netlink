#include <stdarg.h>
#include <stdio.h>
#include <log/log.h>


void log_func_impl(int level, const char *fmt, va_list args) {
		vfprintf(stdout, fmt, args);
		fprintf(stdout, "\n");
		fflush(stdout);
}

void flb_log_handle(int level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_func_impl(level, fmt, args);
  va_end(args);
}