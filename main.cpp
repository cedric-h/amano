#include "platform.h"
#include "math.hpp"

#define LOG_CACHE_SIZE (1 << 11)
static char log_cache[LOG_CACHE_SIZE];
static usize log_cache_length = 0;

void putval(char c) {
  if (log_cache_length >= LOG_CACHE_SIZE) {
    console_log_n("Log cache exhausted", 19);
    return;
  }
  if (c == '\n') {
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

void tprintf(const char* format) // base function
{
  putval(format);
}

// SKEJETON: I know CEDRIC.. you are going to kill me for this:
template<typename T, typename... Targs>
void tprintf(const char* format, T value, Targs... Fargs) 
{
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

PLATFORM_EXPORT void keyhit(bool down, const char *scancode) {
  if (down) {
    tprintf("KEYINFO:OK DOWN '{}'\n", scancode);
  } else {
    tprintf("KEYINFO:OK UP '{}'\n", scancode);
  }
}

float aspect = 1;

PLATFORM_EXPORT void resize(int width, int height) {
  tprintf("RESIZE:OK ({}, {})\n", width, height);
  aspect = width / float(height);
}

PLATFORM_EXPORT void mousemove(int x, int y) {
  tprintf("MOUSEMOVE:OK ({}, {})\n", x, y);
}

PLATFORM_EXPORT void frame(float dt) {
  tprintf("FRAME:OK\n");

  static float theta = 0;
  static u16 indices[] = {
    0, 1, 2, 
    3, 5, 4,
    6, 7, 8,  6, 8, 9
  };
  static Vert vertices[] = {
    {{(-0.5-0.5)/3.0, (0.5+0.5)/3.0, -1.0}, {1.0, 1.0, 0.0}, 1.0},
    {{(-0.5-0.5)/3.0, (-0.5+0.5)/3.0, -1.0}, {1.0, 0.0, 0.0}, 1.0}, 
    {{(0.5-0.5)/3.0, (-0.5+0.5)/3.0, -1.0}, {1.0, 1.0, 0.0}, 1.0},

    {{(-0.5+0.5)/3.0, (0.5-0.5)/3.0, -1.0}, {1.0, 1.0, 0.0}, 1.0},
    {{(0.5+0.5)/3.0, (0.5-0.5)/3.0, -1.0}, {1.0, 0.0, 0.0}, 1.0}, 
    {{(0.5+0.5)/3.0, (-0.5-0.5)/3.0, -1.0}, {1.0, 1.0, 0.0}, 1.0},

    {{-0.5, 0.5, -1.0}, {1.0, 1.0, 0.0}, 1.0},
    {{-0.5, -0.5, -1.0}, {1.0, 0.0, 0.0}, 1.0}, 
    {{0.5, -0.5, -1.0}, {1.0, 1.0, 0.0}, 1.0},
    {{0.5, 0.5, -1.0}, {1.0, 0.0, 0.0}, 1.0},
  };

  theta += dt;

  render(indices, sizeof indices / sizeof indices[0], vertices, sizeof vertices / sizeof vertices[0], math_m4_rotation2d(theta) * math_m4_scale({1/aspect, 1, 1}));
}

PLATFORM_EXPORT void init(void) {
  tprintf("{} is the meaning of life, and {} is the answer to everything.\n", 42, 3.14159265);
  tprintf("INIT:OK\n");
}