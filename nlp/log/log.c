#include <log/log.h>
#include <stdarg.h>
#include <stdio.h>

char *log_color(int level) {
  switch (level) {
  case LOG_LEVEL_CRIT:
    return "\033[1;31m";
  case LOG_LEVEL_ERR:
    return "\033[0;31m";
  case LOG_LEVEL_WARNING:
    return "\033[0;33m";
  case LOG_LEVEL_DEBUG:
    return "\033[0;32m";
  case LOG_LEVEL_TRACE:
    return "\033[0;32m";
  case LOG_LEVEL_INFO:
    return "\033[0;34m";
  default:
    return "";
  }
}

void log_func_impl(int level, const char *fmt, va_list args) {
  fprintf(stdout, "%s", log_color(level));
  vfprintf(stdout, fmt, args);
  fprintf(stdout, "\033[0;0m\n");
  fflush(stdout);
}

void flb_log_handle(int level, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  log_func_impl(level, fmt, args);
  va_end(args);
}