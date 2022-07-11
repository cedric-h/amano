#include "log.h"

char log_cache[LOG_CACHE_SIZE+1];
usize log_cache_length = 0;
bool log_noflush = false;

char *flush() {
  log_cache[log_cache_length] = 0;
  log_cache_length = 0;

  return log_cache;
}

void putval(char c) {
  if (log_cache_length >= LOG_CACHE_SIZE) {
    console_log_n("Log cache exhausted", 19);
    return;
  }
  if (c == '\n' && log_noflush == false) {
    console_log_n(log_cache, log_cache_length);
    log_cache_length = 0;
  } else {
    log_cache[log_cache_length++] = c;
  }
}

void putval(const char *s) {
  while (*s) {
    putval(*s++);
  }
}

void putval(int v) {
  char buf[16] = { 0 };
  int i = 0;

  if (v == 0) {
    putval('0');
    return;
  }

  if (v < 0) {
    v *= -1;
    putval('-');
  }

  while (v > 0) {
    buf[i++] = v % 10 + '0';
    v /= 10;
  }

  while (i--) {
    putval(buf[i]);
  }
}

void putval(double v) {
  if (v < 0) {
    v *= -1;
    putval('-');
  }

  putval(int(v));
  putval('.');
  putval(int((v-int(v)) * 1000000));
}

void putval(float v) {
  putval(double(v));
}

void tprintf(const char* format) {
  putval(format);
}