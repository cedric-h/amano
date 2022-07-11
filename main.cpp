#include "platform.h"
#include "math.h"
#include "gen.h"

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

#define LOG_CACHE_SIZE (1 << 11)
static char log_cache[LOG_CACHE_SIZE+1];
static usize log_cache_length = 0;
static bool log_noflush = false;

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

Vec3 move_relative_to_camera(Camera *cam, Vec3 by) {
  return m4_rotate_y(cam->rotation.y) * m4_rotate_x(cam->rotation.x) * by;
}

Vec3 get_infront_of_camera(Camera *cam, float by) {
  return cam->position + move_relative_to_camera(cam, {0, 0, by});
}

void cam_move(Camera *cam, Vec3 rotation_delta, Vec3 position_delta) {
  const float LIM = MATH_PI_2-0.001;

  cam->rotation += rotation_delta;
  cam->position += m4_rotate_y(cam->rotation.y) * position_delta;

  cam->rotation.x = fmod(cam->rotation.x, MATH_PI*2);
  cam->rotation.y = fmod(cam->rotation.y, MATH_PI*2);

  if (cam->rotation.x > LIM) {
    cam->rotation.x = LIM;
  } else if (cam->rotation.x < -LIM) {
    cam->rotation.x = -LIM;
  }
}

Mat4 cam_vp(Camera *cam) {
  Vec3 eye = m4_rotate_y(cam->rotation.y) * m4_rotate_x(cam->rotation.x) * Vec3{0, 0, 1};
  return m4_perspective(cam->fov, cam->aspect, 0.1, 1000.0) * m4_lookat(cam->position, cam->position+eye, {0, 1, 0});
}

enum Key {
  KEY_SHIFT, KEY_SPACE,
  KEY_W, KEY_A, KEY_S, KEY_D,
  KEY_COUNT
};

struct Object {
  Vec3 pos, rot;
};

struct World {
#define OBJ_MAX 1024
  Object objects[OBJ_MAX];
  usize object_count;
};

struct State {
  float window_w, window_h;
  float aspect;
  Camera cam;
  int old_mouse_x, old_mouse_y;
  bool keys[KEY_COUNT];
  Object leading_obj;
  World world;
} *state;

State state_memory;

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return *a-*b;
}

void place_world_obj(World *world, Object obj) {
  if (world->object_count >= OBJ_MAX) {
    tprintf("Too many obj on screen, can't put\n");
    return;
  }
  world->objects[world->object_count++] = obj;
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

  if (strcmp(scancode, "KeyF") == 0 && down) {
    place_world_obj(&state->world, state->leading_obj);
  }
}

PLATFORM_EXPORT void resize(int width, int height) {
  state->window_w = width;
  state->window_h = height;
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

void render_cube(Vec3 at, Vec3 rot) {
  Mat4 vp = cam_vp(&state->cam);
  Mat4 m = m4_translate(at)*m4_rotate_yxz(rot);
  render(cube_indices, sizeof cube_indices / sizeof cube_indices[0], cube_vertices, sizeof cube_vertices / sizeof cube_vertices[0], vp*m);
}

void memcpy(void *dest, const void *src, usize n) {
  while (n--) {
    *(u8*)dest = *(u8*)src;
  }
}

void render_8x8_bitmap(const u8 *bitmap, float x, float y, float w, float h) {
  Vert base[4] = {
    {{0, 0, -1}, {1, 1, 1}, 1},
    {{0, -1, -1}, {1, 1, 1}, 1},
    {{1, -1, -1}, {1, 1, 1}, 1},
    {{1, 0, -1}, {1, 1, 1}, 1},
  };

  u16 base_indices[6] = {
    0, 2, 1,
    0, 3, 2,
  };

  Vert out[4*8*8];
  int out_count = 0;
  u16 out_indices[6*8*8];
  int out_indices_count = 0;

  for (int x = 0; x < 8; ++x) {
    for (int y = 0; y < 8; ++y) {
      bool set = bitmap[y] & (1 << x);
      if (set) {
        for (int i = 0; i < 4; ++i) {
          Vert vertex = base[i];
          vertex.pos += {float(x), float(-y), 0};
          out[i+out_count] = vertex;
        }
        for (int i = 0; i < 6; ++i) {
          out_indices[i+out_indices_count] = base_indices[i]+out_count;
        }
        out_count += 4;
        out_indices_count += 6;
      }
    }
  }

  w /= 8;
  h /= 8;

  Mat4 rectMat = {
    w/state->window_w*2.0f, 0, 0, 0,
    0, h/state->window_h*2.0f, 0, 0,
    0, 0, 1, 0,
    x/state->window_w*2.0f-1.0f, (-y)/state->window_h*2.0f+1.0f, 0, 1
  };

  render(out_indices, out_indices_count, out, out_count, rectMat);
}

void render_8x16ascii_text(const char *txt, int x, int y) {
  int cx, cy, set;
  int init_x = x;
  int column = 0;
  for (;*txt; txt++) {
    if (*txt == '\n') {
      x = init_x;
      y += 18;
      column = 0;
    } else if (*txt == '\t') {
      x += 8 * (2 - (column % 2));
      column += 2 - (column % 2);
    } else {
      column += 1;
      render_8x8_bitmap(gen_font8x8_basic.data[*txt], x, y, 8, 16);
      x += 8;
    }
  }
}

// NOTE: THIS IS A HACK!
#define PUT_DEBUG_TEXT(x, y, args...) do {log_noflush = true; tprintf(args); log_noflush = false; render_8x16ascii_text(flush(), (x), (y));} while(0);

void render_debug_info(float dt) {
  Vec3 pos = state->cam.position;
  Vec3 rot = state->cam.rotation;

  PUT_DEBUG_TEXT(20, 20, 
    "- Debug Info\n"
    "\t> Object count: {}\n"
    "\t- Camera\n"
    "\t\t> Position: ({}, {}, {})\n"
    "\t\t> Rotation: ({}, {})\n"
    "\t> DeltaTime: {}s\n", 
    int(state->world.object_count),
    pos.x, pos.y, pos.z,
    int(rot.x*(180/MATH_PI)), int(rot.y*(180/MATH_PI)),
    dt);
}

PLATFORM_EXPORT void frame(float dt) {
  cam_move(&state->cam, {0, 0, 0}, 
    {(state->keys[KEY_A] - state->keys[KEY_D]) * dt,
     (state->keys[KEY_SPACE] - state->keys[KEY_SHIFT]) * dt,
     (state->keys[KEY_S] - state->keys[KEY_W]) * dt});

  static float theta = 0;

  theta += dt;
  for (int i = 0; i < state->world.object_count; ++i) {
    Object *obj = &state->world.objects[i];
    render_cube(obj->pos, obj->rot);
  }

  state->leading_obj.pos = get_infront_of_camera(&state->cam, -5);
  state->leading_obj.rot = {0, state->cam.rotation.y, 0};

  render_cube(state->leading_obj.pos, state->leading_obj.rot);
  render_debug_info(dt);
}

PLATFORM_EXPORT void init(void) {
  state = &state_memory;
  state->aspect = state->cam.aspect = 1;
  state->cam.fov = MATH_PI_2/4;
  state->cam.position.z = 5;
}
