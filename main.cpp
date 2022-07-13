#include "platform.h"
#include "math.h"
#include "gen.h"
#include "log.h"

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

enum Item {
  Item_Wood,
  Item_COUNT,
};

const char *item_names[] = {
  /*[Item_Wood] = */ "Wood"
};

struct Inventory {
  int items[Item_COUNT];
};

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
             Vec3{0, 0, 1};
  return m4_perspective(cam->fov, cam->aspect, 0.1, 1000.0) * m4_lookat(cam->position, cam->position+eye, {0, 1, 0});
}

struct Ray {
  Vec3 origin;
  Vec3 direction;
};

struct Box {
  Vec3 min;
  Vec3 max;
};

enum Key {
  Key_Shift, Key_Space,
  Key_W, Key_A, Key_S, Key_D,
  Key_COUNT
};

enum Shape {
  Shape_Cube,
  Shape_Cylinder,
  Shape_COUNT
};

struct Object {
  Shape shape;
  Item drop;
  Vec3 pos, rot, scale;
  bool exists;
  bool unbreakable;
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
  bool keys[Key_COUNT];

  /* sim */
  bool is_placing_floor;
  Object placing_obj;
  Object *facing_obj; 

  World world;
  Inventory inventory;

} *state;

State state_memory;

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) {
    a++;
    b++;
  }
  return *a-*b;
}


Object default_obj(Shape shape) {
  Object result = {};
  result.shape = shape;
  result.scale = {1.0, 1.0, 1.0};
  result.drop = Item_COUNT;
  return result;
}

void place_world_obj(World *world, Object obj) {
  if (world->object_count >= OBJ_MAX) {
    tprintf("Too many obj on screen, can't put\n");
    return;
  }
  obj.exists = true;
  world->objects[world->object_count++] = obj;
}

bool is_object_valid(Object *object) {
  return object != nullptr && object->exists;
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

void render_shape_colored(Shape shape, Mat4 m, float color) {
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

  // Reassign normals. NOTE: mutates original geo normals!
  for (int i = 0; i < src->ibuf_len; i += 3) {
    Vert *a = &src->vbuf[src->ibuf[i]],
         *b = &src->vbuf[src->ibuf[i+1]],
         *c = &src->vbuf[src->ibuf[i+2]];
    Vec3 ap = a->pos * m,
         bp = b->pos * m,
         cp = c->pos * m;

    Vec3 normal = v3_normalize(v3_cross(ap-cp, bp-cp));
    a->norm = b->norm = c->norm = normal;
  }


  int v_start = fgeo.vbuf_len;
  for (int i = 0; i < src->vbuf_len; i++) {
    Vert vert = src->vbuf[i];
    vert.norm = vert.norm;
    vert.color = color;
    vert.pos = m * vert.pos;
    fgeo.vbuf[fgeo.vbuf_len++] = vert;
  }

  for (int i = 0; i < src->ibuf_len; i++) {
    fgeo.ibuf[fgeo.ibuf_len++] = v_start + src->ibuf[i];
  }
}

void render_shape_colored(Shape shape, Vec3 at, Vec3 rot, float color) {
  render_shape_colored(shape, m4_translate(at)*m4_rotate_yxz(rot), color);
}

void render_shape(Shape shape, Mat4 m) {
  render_shape_colored(shape, m, 1.0f);
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

float object_distance(Object *a, Object *b) {
  return v3_length(a->pos - b->pos);
}

float object_distance(Object *a, Vec3 b) {
  return v3_length(a->pos - b);
}

Mat4 object_model(Object *obj) {
  return m4_translate(obj->pos) * m4_scale(obj->scale) * m4_rotate_yxz(obj->rot);
}

Object *find_focus_obj(Vec3 to) {
  Object *closest = nullptr;
  float closest_distance = 1000000;

  for (int i = 0; i < state->world.object_count; ++i) {
    Object *obj = &state->world.objects[i];
    // NOTE: checking if position is < 1.0f ensures that we don't place on top of trunks and other second layer objs
    // This is probably a temporary hack.
    if (obj->exists && obj->pos.y < 1.0f) { 
      float distance = v3_length(state->world.objects[i].pos - to);
      if (distance < 4 && distance < closest_distance)  {
        closest_distance = distance;
        closest = &state->world.objects[i];
      }
    }
  }

  return closest;
}

float normalize_angle(float angle) {
  while (angle < 0) {
    angle += MATH_TAU;
  }
  angle = fmod(angle, MATH_TAU);

  return angle;
}

float granular_rotation(float origin, float angle, int segments) {
  angle = normalize_angle(angle);
  origin = normalize_angle(origin);
  const float segment_size = (MATH_TAU) / segments;
  return normalize_angle(__builtin_round((angle - origin) / (MATH_TAU) * segments) * segment_size + origin);
}

Object make_aligned_object(Object *focus, Camera *cam) {
  Object result = default_obj(Shape_Cube);
  result.drop = Item_Wood;
  float rotation;

  switch (focus->shape) {
    case Shape_Cube:
      rotation = granular_rotation(focus->rot.y, cam->rotation.y, 4);
      break;
    case Shape_Cylinder:
      rotation = cam->rotation.y;
      break;
    default: 
      break;
  }

  result.pos = Vec3{0, 0, 1} * m4_rotate_y(rotation) + find_focus_obj(state->cam.position)->pos;
  result.rot.y = rotation;
  return result;
}

Box expand_box_from_point(Vec3 point, float radius) {
  return {{point.x - radius, point.y - radius, point.z - radius},
          {point.x + radius, point.y + radius, point.z + radius}};
}

Vec3 box_origin(Box box) {
  return {(box.max.x + box.min.x)/2, (box.max.y + box.min.y)/2, (box.max.z + box.min.z)/2};
}

bool point_vs_box(Vec3 pos, Box box) {
  return pos.x > box.min.x && pos.x < box.max.x &&
         pos.y > box.min.y && pos.y < box.max.y &&
         pos.z > box.min.z && pos.z < box.max.z;
}

bool ray_vs_box(Ray ray, Box box, Mat4 m) {
  float delta = 0.01;
  Vec3 pos = ray.origin;
  Vec3 dir = v3_normalize(ray.direction) * delta;
  Vec3 origin = box_origin(box);
  Box centerBox = box;
  centerBox.min.x -= origin.x;
  centerBox.min.y -= origin.y;
  centerBox.min.z -= origin.z;
  centerBox.max.x -= origin.x;
  centerBox.max.y -= origin.y;
  centerBox.max.z -= origin.z;

  for (int i = 0; i < 100; ++i) {
    pos += dir * i;
    Vec3 transformed = (origin - pos) * m;

    if (point_vs_box(transformed, centerBox)) {
      return true;
    }
  }
  return false;
}

Ray cam_ray(Camera *cam) {
  return {cam->position, m4_rotate_y(cam->rotation.y) * m4_rotate_x(cam->rotation.x) * Vec3{0, 0, 1}};
}

void render_overlays(float dt) {
  // Debug
  render_debug_info(dt);

  // Cursor
  const u8 cursor_bitmap[] = {
    0b00010000,
    0b01010100,
    0b00010000,
    0b11111110,
    0b00010000,
    0b01010100,
    0b00010000,
    0b00000000,
  };
  render_8x8_bitmap(cursor_bitmap, state->window_w/2-8,  state->window_h/2-8, 16, 16);

  // Inventory
  for (int i = 0; i < Item_COUNT; ++i) {
    PUT_DEBUG_TEXT(20 + i * 100, state->window_h-32, "{} x{}", item_names[i], state->inventory.items[i]);
  }
}

void render_obj(Object *obj, float color = 1.0f) {
  render_shape_colored(obj->shape, object_model(obj), color);
}

PLATFORM_EXPORT void frame(float dt) {
  cam_move(&state->cam, {0, 0, 0}, 
    {(state->keys[Key_D] - state->keys[Key_A]) * dt,
     (state->keys[Key_Space] - state->keys[Key_Shift]) * dt,
     (state->keys[Key_W] - state->keys[Key_S]) * dt});

  static float theta = 0;

  fgeo_flush_generation = 0;
  theta += dt;

  state->facing_obj = nullptr;
  state->is_placing_floor = false;

  if (state->inventory.items[Item_Wood] > 0) {
    Object *focus = find_focus_obj(state->cam.position);
    if (focus != nullptr) {
      state->placing_obj = make_aligned_object(focus, &state->cam);
      Object *maybe_occlusion = find_focus_obj(state->placing_obj.pos);
      if (object_distance(&state->placing_obj, maybe_occlusion) > 0.99) { // find if we placed the block here (ERROR PRONE TEMPORARY)
        state->is_placing_floor = true;
      }
    }
  }

  for (int i = 0; i < state->world.object_count; ++i) {
    Object *obj = &state->world.objects[i];
    if (obj->exists) {
      Mat4 transform = m4_rotate_yxz(obj->rot) * m4_scale({1/obj->scale.x, 1/obj->scale.y, 1/obj->scale.z});
      if (ray_vs_box(cam_ray(&state->cam), expand_box_from_point(obj->pos, 0.5), transform)) {
        if (state->facing_obj == nullptr) {
          state->is_placing_floor = false;
          state->facing_obj = obj;
        } else {
          float distance_a = object_distance(state->facing_obj, state->cam.position);
          float distance_b = object_distance(obj, state->cam.position);
          if (distance_b < distance_a) {
            render_obj(state->facing_obj);
            state->is_placing_floor = false;
            state->facing_obj = obj;
          } else {
            render_obj(obj);
          }
        }
      } else {
        render_obj(obj);
      }
    }
  }

  if (state->facing_obj) {
    render_obj(state->facing_obj, 0.3);
  }

  if (state->is_placing_floor) {
    render_obj(&state->placing_obj, 0.5);
  }

  render_overlays(dt);

  flush_fgeo();
}

PLATFORM_EXPORT void init(void) {
  {
    Geo *geo = shape_geos + Shape_Cylinder;
    auto vert = [geo](float x, float y, float z, Vec3 norm = {0, 0, 1}) {
      Vert v = {0};
      v.pos = Vec3{x, y, z};
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
  state->cam.fov = MATH_PI_2/2;
  state->cam.position.y = 2;
  state->inventory.items[Item_Wood] = 0;

  Object dirt_obj = default_obj(Shape_Cylinder);
  dirt_obj.unbreakable = true;

  place_world_obj(&state->world, dirt_obj);

  Object tree_obj = default_obj(Shape_Cube);
  tree_obj.drop = Item_Wood;
  tree_obj.unbreakable = true;
  tree_obj.scale = {0.2, 1.0, 0.2};
  tree_obj.pos.y = 1.0f;

  place_world_obj(&state->world, tree_obj);
}

PLATFORM_EXPORT void keyhit(bool down, const char *scancode) {
  const char *map[Key_COUNT];
  map[Key_Shift] = "ShiftLeft";
  map[Key_Space] = "Space";
  map[Key_W] = "KeyW";
  map[Key_S] = "KeyS";
  map[Key_A] = "KeyA";
  map[Key_D] = "KeyD";

  for (int i = 0; i < Key_COUNT; ++i) {
    if (strcmp(scancode, map[i]) == 0) {
      state->keys[i] = down;
    }
  }
}

PLATFORM_EXPORT void resize(int width, int height) {
  state->window_w = width;
  state->window_h = height;
  state->aspect = state->cam.aspect = width / float(height);
}

PLATFORM_EXPORT void mousemove(int x, int y, int dx, int dy) {
  cam_move(&state->cam, {float(dy)/300.0f, float(dx)/300.0f, 0}, {0, 0, 0});
}

PLATFORM_EXPORT void mousehit(bool down, int button) {
  // Wall placement
  if (down && button == 2 && is_object_valid(state->facing_obj) && state->inventory.items[Item_Wood] > 0) {
    Object new_obj = default_obj(Shape_Cube);
    new_obj.drop = Item_Wood;
    new_obj.rot = state->facing_obj->rot;
    new_obj.scale.y = 2.0;
    new_obj.pos = state->facing_obj->pos + Vec3{0, 1.5, 0};
    state->inventory.items[Item_Wood] -= 1;
    place_world_obj(&state->world, new_obj);
  }
  if (down && button == 0 && is_object_valid(state->facing_obj)) {
    if (state->facing_obj->unbreakable == false) {
      state->facing_obj->exists = false;
    }
    state->inventory.items[state->facing_obj->drop] += 1;
  }
  if (state->is_placing_floor && down && button == 2 && state->inventory.items[Item_Wood] > 0) {
    state->inventory.items[Item_Wood] -= 1;
    place_world_obj(&state->world, state->placing_obj);
  }
}


