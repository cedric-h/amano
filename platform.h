#ifndef PLATFORM_H
#define PLATFORM_H
// <WASM ONLY>
#define PLATFORM_EXPORT __attribute__((visibility("default"))) extern "C"
#define PLATFORM_IMPORT extern "C"

static_assert(sizeof(char) == 1, "sizeof(char) != 1");
typedef char i8;
typedef unsigned char u8;

static_assert(sizeof(short) == 2, "sizeof(short) != 2");
typedef short i16;
typedef unsigned short u16;

static_assert(sizeof(int) == 4, "sizeof(int) != 4");
typedef int i32;
typedef unsigned int u32;

typedef u32 usize;
typedef i32 isize;
// </WASM ONLY>

struct Mat4 {
  float num[4][4];
};

struct Vec3 {
  float x, y, z;
};

struct __attribute__((packed)) Vert {
  Vec3 pos, norm;
  float color;
}; 

/* this is needed for interop of strings between JS and C++ */
/* this is a temporary bump allocator, so upon calling setstack you might free something you didn't want to, so be careful! */
PLATFORM_EXPORT void* getstack(usize n);
PLATFORM_EXPORT void setstack(void* at);

PLATFORM_EXPORT void init(void);

/* events */
PLATFORM_EXPORT void frame(float dt); // expected to call `render`
PLATFORM_EXPORT void keyhit(bool down, const char *scancode);
PLATFORM_EXPORT void resize(int width, int height);
PLATFORM_EXPORT void mousemove(int x, int y);

PLATFORM_IMPORT void render(
  u16  *indexes, int index_count,
  Vert *verts,   int vert_count,
  Mat4  mvp
);

PLATFORM_IMPORT void console_log_n(const char *string, usize strlen);
#endif