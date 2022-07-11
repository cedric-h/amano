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
  Vec3 rotation; // don't use the z component (just don't)
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
  Vec3 eye = m4_rotate_y(cam->rotation.y) *
             m4_rotate_x(cam->rotation.x) *
             Vec3{0, 0, 1}                ;
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
#define OBJ_MAX 64
  Object objects[OBJ_MAX];
  usize object_count;
};

struct State {
  /* view */
  float aspect;
  Camera cam;

  /* input */
  int old_mouse_x, old_mouse_y;
  bool keys[KEY_COUNT];

  /* sim */
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

  if (strcmp(scancode, "KeyF") == 0 && !down) {
    place_world_obj(&state->world, state->leading_obj);
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

struct Geo {
  int ibuf_len, vbuf_len;
  u16 *ibuf;
  Vert *vbuf;
};
static u16 geo_push_vert(Geo *geo, Vert v) {
  u16 i = geo->vbuf_len;
  geo->vbuf[geo->vbuf_len++] = v;
  return i;
}
static void geo_make_tri(Geo *geo, u16 a, u16 b, u16 c) {
  geo->ibuf[geo->ibuf_len++] = a;
  geo->ibuf[geo->ibuf_len++] = b;
  geo->ibuf[geo->ibuf_len++] = c;
}


static u16  __frame_ibuf[1 << 11];
static Vert __frame_vbuf[1 << 11];
Geo fgeo = {
  .ibuf = __frame_ibuf,
  .vbuf = __frame_vbuf,
};

enum Shape {
  Shape_Cube,
  Shape_Cylinder,
  Shape_COUNT
};
static u16  __cyl_ibuf[1 << 5];
static Vert __cyl_vbuf[1 << 6];
Geo shape_geos[Shape_COUNT] = {
  /*[Shape_Cube] = */{
    .ibuf_len = sizeof cube_indices / sizeof cube_indices[0],
    .vbuf_len = sizeof cube_vertices / sizeof cube_vertices[0],
    .ibuf = cube_indices,
    .vbuf = cube_vertices,
  },
  /*[Shape_Cylinder] = */{
    .ibuf = __cyl_ibuf,
    .vbuf = __cyl_vbuf
  },
};

void render_shape(Shape shape, Vec3 at, Vec3 rot) {
  Mat4 m = m4_translate(at)*m4_rotate_yxz(rot);
  Geo *src = shape_geos + shape;

  int v_start = fgeo.vbuf_len;
  for (int i = 0; i < src->vbuf_len; i++) {
    Vert vert = src->vbuf[i];
    vert.pos = m * vert.pos;
    fgeo.vbuf[fgeo.vbuf_len++] = vert;
  }

  for (int i = 0; i < src->ibuf_len; i++)
    fgeo.ibuf[fgeo.ibuf_len++] = v_start + src->ibuf[i];
}

PLATFORM_EXPORT void frame(float dt) {
  cam_move(&state->cam, {0, 0, 0}, 
    {(state->keys[KEY_A] - state->keys[KEY_D]) * dt,
     (state->keys[KEY_SPACE] - state->keys[KEY_SHIFT]) * dt,
     (state->keys[KEY_S] - state->keys[KEY_W]) * dt});

  static float theta = 0;

  fgeo.vbuf_len = 0;
  fgeo.ibuf_len = 0;

  theta += dt;
  for (int i = 0; i < state->world.object_count; ++i) {
    Object *obj = &state->world.objects[i];
    render_shape(Shape_Cube, obj->pos, obj->rot);
  }

  state->leading_obj.pos = get_infront_of_camera(&state->cam, -5);
  state->leading_obj.rot = {0, state->cam.rotation.y, 0};

  render_shape(Shape_Cylinder, state->leading_obj.pos, state->leading_obj.rot);

  render(
    fgeo.ibuf, fgeo.ibuf_len,
    fgeo.vbuf, fgeo.vbuf_len,
    cam_vp(&state->cam)
  );
}

PLATFORM_EXPORT void init(void) {
  {
    Geo *geo = shape_geos + Shape_Cylinder;
    auto vert = [geo](float x, float y, float z) {
      Vert v = {0};
      v.pos = Vec3 { x, y, z };
      v.color = 0.5f;
      return geo_push_vert(geo, v);
    };
    u16 fan_center_bottom = vert(0.0f, 0.0f, 0.0f),
        fan_center_top    = vert(0.0f, 2.0f, 0.0f);
    for (float i = 0.0f; i < 10.0f; i++) {
        float t0 =  i         / 10.0f * MATH_TAU,
              t1 = (i + 1.0f) / 10.0f * MATH_TAU;
        u16 bottom_l = vert(sin(t0)*0.5f, 0.0f, cos(t0)*0.5f),
            bottom_r = vert(sin(t1)*0.5f, 0.0f, cos(t1)*0.5f),
               top_l = vert(sin(t0)*0.5f, 1.0f, cos(t0)*0.5f),
               top_r = vert(sin(t1)*0.5f, 1.0f, cos(t1)*0.5f);
        geo_make_tri(geo, bottom_l, bottom_r, top_l);
        geo_make_tri(geo, bottom_r, top_r, top_l);

        geo_make_tri(geo, top_l, top_r, fan_center_top);
        geo_make_tri(geo, bottom_l, fan_center_bottom, bottom_r);
    }
  }

  state = &state_memory;
  state->aspect = state->cam.aspect = 1;
  state->cam.fov = MATH_PI_2/4;
  state->cam.position.z = 5;
}
