#include "platform.h"
#include "math.hpp"

#define LOG_CACHE_SIZE (1 << 11)
static char log_cache[LOG_CACHE_SIZE];
static usize log_cache_length = 0;

Vert cube_vertices[] = {
  // pos                normal    value
  // +y
  {{-0.5, 0.5, -0.5},     {0, 1, 0},  1},
  {{-0.5, 0.5, 0.5},      {0, 1, 0},  1},
  {{0.5, 0.5, 0.5},       {0, 1, 0},  1},
  {{0.5, 0.5, -0.5},      {0, 1, 0},  1},

  // -x
  {{-0.5, 0.5, 0.5},      {-1, 0, 0}, 1},
  {{-0.5, -0.5, 0.5},     {-1, 0, 0}, 1}, 
  {{-0.5, -0.5, -0.5},    {-1, 0, 0}, 1},
  {{-0.5, 0.5, -0.5},     {-1, 0, 0}, 1},

  // +x
  {{0.5, 0.5, 0.5},       {1, 0, 0}, 1}, 
  {{0.5, -0.5, 0.5},      {1, 0, 0}, 1}, 
  {{0.5, -0.5, -0.5},     {1, 0, 0}, 1}, 
  {{0.5, 0.5, -0.5},      {1, 0, 0}, 1}, 

  // -z
  {{0.5, 0.5, 0.5},       {0, 0, -1}, 1},
  {{0.5, -0.5, 0.5},      {0, 0, -1}, 1},
  {{-0.5, -0.5, 0.5},     {0, 0, -1}, 1},
  {{-0.5, 0.5, 0.5},      {0, 0, -1}, 1},

  // +z
  {{0.5, 0.5, -0.5},       {0, 0, 1}, 1},
  {{0.5, -0.5, -0.5},      {0, 0, 1}, 1},
  {{-0.5, -0.5, -0.5},     {0, 0, 1}, 1},
  {{-0.5, 0.5, -0.5},      {0, 0, 1}, 1},

  // -y
  {{-0.5, -0.5, -0.5},     {0, -1, 0}, 1},
  {{-0.5, -0.5, 0.5},      {0, -1, 0}, 1},
  {{0.5, -0.5, 0.5},       {0, -1, 0}, 1},
  {{0.5, -0.5, -0.5},      {0, -1, 0}, 1},
};

u16 cube_indices[] = {
  0, 1, 2,  0, 2, 3,       // +y
  6, 5, 4,  7, 6, 4,       // -x
  8, 9, 10,  8, 10, 11,    // +x
  14, 13, 12,  15, 14, 12, // -z
  16, 17, 18,  16, 18, 19, // +z
  22, 21, 20,  23, 22, 20  // -y
};

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

void tprintf(const char* format) {
  putval(format);
}

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

struct Camera {
  Vec3 rotation; // don't use the z component
  Vec3 position;
  float fov;
  float aspect;
};


void cam_move(Camera *cam, Vec3 rotation_delta, Vec3 position_delta) {
  const float LIM = MATH_PI_2-0.001;

  cam->position += m4_rotate_y(cam->rotation.y) * position_delta;
  cam->rotation += rotation_delta;

  cam->rotation.x = fmod(cam->rotation.x, MATH_PI*2);
  cam->rotation.y = fmod(cam->rotation.y, MATH_PI*2);

  if (cam->rotation.x > LIM) {
    cam->rotation.x = LIM;
  } else if (cam->rotation.x < -LIM) {
    cam->rotation.x = -LIM;
  }
}

Mat4 cam_vp(Camera *cam) {
  Vec3 eye = v3_normalize(m4_rotate_y(cam->rotation.y) * m4_rotate_x(cam->rotation.x) * Vec3{0, 0, 1});
  return m4_perspective(cam->fov, cam->aspect, 0.1, 1000.0) * m4_lookat(cam->position, cam->position+eye, {0, 1, 0});
}

enum Key {
  KEY_SHIFT, KEY_SPACE,
  KEY_W, KEY_A, KEY_S, KEY_D,
  KEY_COUNT
};

struct State {
  float aspect;
  Camera cam;
  int old_mouse_x, old_mouse_y;
  bool keys[KEY_COUNT];
} *state;

State state_memory;

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return *a-*b;
}

PLATFORM_EXPORT void keyhit(bool down, const char *scancode) {
  const char *map[KEY_COUNT];
  map[KEY_SHIFT] = "ShiftLeft";
  map[KEY_SPACE] = "Space";
  map[KEY_W] = "KeyW";
  map[KEY_S] = "KeyS";
  map[KEY_A] = "KeyA";
  map[KEY_D] = "KeyD";

  for (int i = 0; i < KEY_COUNT; ++i) {
    if (strcmp(scancode, map[i]) == 0) {
      state->keys[i] = down;
    }
  }
}

PLATFORM_EXPORT void resize(int width, int height) {
  state->aspect = state->cam.aspect = width / float(height);
}

PLATFORM_EXPORT void mousemove(int x, int y) {
  int dx = x - state->old_mouse_x, dy = y - state->old_mouse_y;
  if (!(state->old_mouse_x == 0 && state->old_mouse_y == 0)) { // fix moving from 0, 0
    cam_move(&state->cam, {-float(dy)/300.0f, float(dx)/300.0f, 0}, {0, 0, 0});
  }
  state->old_mouse_x = x;
  state->old_mouse_y = y;
}


PLATFORM_EXPORT void frame(float dt) {
  cam_move(&state->cam, {0, 0, 0}, 
    {(state->keys[KEY_A] - state->keys[KEY_D]) * dt,
     (state->keys[KEY_SPACE] - state->keys[KEY_SHIFT]) * dt,
     (state->keys[KEY_S] - state->keys[KEY_W]) * dt});

  static float theta = 0;

  theta += dt;

  render(cube_indices, sizeof cube_indices / sizeof cube_indices[0], cube_vertices, sizeof cube_vertices / sizeof cube_vertices[0], cam_vp(&state->cam));
}

PLATFORM_EXPORT void init(void) {
  state = &state_memory;
  state->aspect = state->cam.aspect = 1;
  state->cam.fov = MATH_PI_2/4;
  state->cam.position.z = 5;
  state->cam.rotation.y = 0;
}
