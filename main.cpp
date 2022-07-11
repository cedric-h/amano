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
#define OBJ_MAX 1024
  Object objects[OBJ_MAX];
  usize object_count;
};

struct State {
  /* view */
  float window_w, window_h;
  float aspect;
  Camera cam;

  /* input */
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

  if (strcmp(scancode, "KeyF") == 0 && down) {
    place_world_obj(&state->world, state->leading_obj);
  }
}

PLATFORM_EXPORT void resize(int width, int height) {
  state->window_w = width;
  state->window_h = height;
  state->aspect = state->cam.aspect = width / float(height);
}

PLATFORM_EXPORT void mousemove(int x, int y, int dx, int dy) {
  cam_move(&state->cam, {-float(dy)/300.0f, float(dx)/300.0f, 0}, {0, 0, 0});
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


#define FRAME_VBUF_SIZE (1 << 12)
#define FRAME_IBUF_SIZE (1 << 13)

static u16  __frame_ibuf[FRAME_IBUF_SIZE];
static Vert __frame_vbuf[FRAME_VBUF_SIZE];
Geo fgeo = {
  .ibuf = __frame_ibuf,
  .vbuf = __frame_vbuf,
};
static int fgeo_flush_generation = 0;

enum Shape {
  Shape_Cube,
  Shape_Cylinder,
  Shape_COUNT
};
static u16  __cyl_ibuf[1 << 7];
static Vert __cyl_vbuf[1 << 7];

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

void flush_fgeo() {
  render(
    fgeo.ibuf, fgeo.ibuf_len,
    fgeo.vbuf, fgeo.vbuf_len,
    cam_vp(&state->cam)
  );
  fgeo.vbuf_len = 0;
  fgeo.ibuf_len = 0;
  fgeo_flush_generation += 1;
}

void render_shape(Shape shape, Mat4 m) {
  Geo *src = shape_geos + shape;

  if ((src->ibuf_len + fgeo.ibuf_len) >= FRAME_IBUF_SIZE) {
    flush_fgeo();
    if ((src->ibuf_len + fgeo.ibuf_len) >= FRAME_IBUF_SIZE) {
      tprintf("Shape {} is too big to fit\n", shape);
      return;
    }
  }
  if ((src->vbuf_len + fgeo.vbuf_len) >= FRAME_VBUF_SIZE) {
    flush_fgeo();
    if ((src->vbuf_len + fgeo.vbuf_len) >= FRAME_VBUF_SIZE) {
      tprintf("Shape {} is too big to fit\n", shape);
      return;
    }
  }

  int v_start = fgeo.vbuf_len;
  for (int i = 0; i < src->vbuf_len; i++) {
    Vert vert = src->vbuf[i];
    vert.pos = m * vert.pos;
    fgeo.vbuf[fgeo.vbuf_len++] = vert;
  }

  for (int i = 0; i < src->ibuf_len; i++) {
    fgeo.ibuf[fgeo.ibuf_len++] = v_start + src->ibuf[i];
  }
}

void render_shape(Shape shape, Vec3 at, Vec3 rot) {
  render_shape(shape, m4_translate(at)*m4_rotate_yxz(rot));
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
      y += 16;
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
    "\t> DeltaTime: {}s\n" 
    "\t> GeoIndices: {}\n" 
    "\t> GeoVertices: {}\n"
    "\t> GeoFlushGen: {}\n", 
    int(state->world.object_count),
    pos.x, pos.y, pos.z,
    int(rot.x*(180/MATH_PI)), int(rot.y*(180/MATH_PI)),
    dt,
    int(fgeo.ibuf_len),
    int(fgeo.vbuf_len),
    fgeo_flush_generation);
}

PLATFORM_EXPORT void frame(float dt) {
  cam_move(&state->cam, {0, 0, 0}, 
    {(state->keys[KEY_A] - state->keys[KEY_D]) * dt,
     (state->keys[KEY_SPACE] - state->keys[KEY_SHIFT]) * dt,
     (state->keys[KEY_S] - state->keys[KEY_W]) * dt});

  static float theta = 0;

  fgeo_flush_generation = 0;
  theta += dt;
  for (int i = 0; i < state->world.object_count; ++i) {
    Object *obj = &state->world.objects[i];
    render_shape(Shape_Cylinder, obj->pos, obj->rot);
  }

  state->leading_obj.pos = get_infront_of_camera(&state->cam, -5);
  state->leading_obj.rot = {0, state->cam.rotation.y, 0};

  render_shape(Shape_Cylinder, state->leading_obj.pos, state->leading_obj.rot);
  render_debug_info(dt);

  //render_shape(Shape_Cube, state->leading_obj.pos, state->leading_obj.rot);
  flush_fgeo();

}

PLATFORM_EXPORT void init(void) {
  {
    Geo *geo = shape_geos + Shape_Cylinder;
    auto vert = [geo](float x, float y, float z, Vec3 norm = {0, 0, 1}) {
      Vert v = {0};
      v.pos = Vec3 { x, y, z };
      v.color = 0.5f;
      v.norm = norm;
      return geo_push_vert(geo, v);
    };
    u16 fan_center_bottom = vert(0.0f, -0.5f, 0.0f, {0, -1, 0}),
        fan_center_top    = vert(0.0f, 0.5f, 0.0f, {0, 1, 0});

    u16 bottom_l = vert(sin(0)*0.5f, -0.5f, cos(0)*0.5f),
        top_l = vert(sin(0)*0.5f, 0.5f, cos(0)*0.5f);
        ;

    for (float i = 1.0f; i <= 10.0f; i += 1.0f) {
        float t =  i         / 10.0f * MATH_TAU;
        u16 bottom_r = vert(sin(t)*0.5f, -0.5f, cos(t)*0.5f),
            top_r = vert(sin(t)*0.5f, 0.5f, cos(t)*0.5f);
        geo_make_tri(geo, bottom_r, top_r, top_l);
        geo_make_tri(geo, bottom_l, bottom_r, top_l);
        geo_make_tri(geo, bottom_l, fan_center_bottom, bottom_r);
        geo_make_tri(geo, top_l, top_r, fan_center_top);
        bottom_l = bottom_r;
        top_l = top_r;
    }
  }

  state = &state_memory;
  state->aspect = state->cam.aspect = 1;
  state->cam.fov = MATH_PI_2/4;
  state->cam.position.z = 0;
}
