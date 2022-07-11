#ifndef LOG_H
#define LOG_H

#include "platform.h"

#define LOG_CACHE_SIZE (1 << 11)
extern char log_cache[LOG_CACHE_SIZE+1];
extern usize log_cache_length;
extern bool log_noflush;

char *flush();
void putval(char c);
void putval(const char *s);
void putval(int v);
void putval(double v);
void putval(float v);
void tprintf(const char* format);

// SKEJETON: I know CEDRIC.. you are going to kill me for this:
template<typename T, typename... Targs>
void tprintf(const char* format, T value, Targs... Fargs) {
  for (; *format != '\0'; format++) {
    if (*format == '{' && format[1] == '{') {
      format += 1;
    } else if (*format == '}' && format[1] == '}') {
      format += 1;
    } else if (*format == '{' && format[1] == '}') {
      putval(value);
      tprintf(format + 2, Fargs...); 
      return;
    } 
    putval(*format);
  }
}

#endif