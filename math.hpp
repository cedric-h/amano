#include "platform.h"


#define MATH_PI 3.141592653589793
#define MATH_PI_2 1.5707963267948966/2.0


#define cos(x) __builtin_cos(x)
#define sin(x) __builtin_sin(x)
#define fmod(x) __builtin_fmod(x)
#define sqrt(x) __builtin_sqrt(x)

/* Vec3 */

inline Vec3 operator+(Vec3 a, Vec3 b) {
  return {a.x+b.x, a.y+b.y, a.z+b.z};
}

inline Vec3 operator-(Vec3 a, Vec3 b) {
  return {a.x-b.x, a.y-b.y, a.z-b.z};
}

inline Vec3 operator*(Vec3 a, Vec3 b) {
  return {a.x*b.x, a.y*b.y, a.z*b.z};
}

inline Vec3 operator/(Vec3 a, Vec3 b) {
  return {a.x/b.x, a.y/b.y, a.z/b.z};
}

inline Vec3 operator*(Vec3 a, float b) {
  return {a.x*b, a.y*b, a.z*b};
}

inline Vec3 operator/(Vec3 a, float b) {
  return {a.x/b, a.y/b, a.z/b};
}

inline Vec3 operator+=(Vec3 &a, Vec3 b) {
  return a = {a.x+b.x, a.y+b.y, a.z+b.z};
}

inline Vec3 operator-=(Vec3 &a, Vec3 b) {
  return a = {a.x-b.x, a.y-b.y, a.z-b.z};
}

inline Vec3 operator*=(Vec3 &a, Vec3 b) {
  return a = {a.x*b.x, a.y*b.y, a.z*b.z};
}

inline Vec3 operator/=(Vec3 &a, Vec3 b) {
  return a = {a.x/b.x, a.y/b.y, a.z/b.z};
}

inline Vec3 operator*=(Vec3 &a, float b) {
  return a = {a.x*b, a.y*b, a.z*b};
}

inline Vec3 operator/=(Vec3 &a, float b) {
  return a = {a.x/b, a.y/b, a.z/b};
}

inline float v3_dot(Vec3 a, Vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline float v3_length(Vec3 a) {
  return sqrt(v3_dot(a, a));
} 

inline Vec3 v3_normalize(Vec3 a) {
  return a / v3_length(a);
}

inline Vec3 v3_cross(Vec3 a, Vec3 b) {
  return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
}

/* Mat4 */

inline Mat4 operator*(Mat4 a, Mat4 b) {
  Mat4 ret = {};

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      for (int k = 0; k < 4; ++k) {
        ret.num[i][j] += a.num[k][j] * b.num[i][k];
      }
    }
  }

  return ret;
}

inline Mat4 operator*=(Mat4 &a, Mat4 b) {
  return a = a * b;
}

inline Mat4 m4_scale(Vec3 scale)
{
  return {
    scale.x, 0,       0,       0,
    0,       scale.y, 0,       0,
    0,       0,       scale.z, 0,
    0,       0,       0,       1
  };
}

inline Mat4 m4_identity() {
  return {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
}

inline Mat4 m4_rotation2d(float theta) {
  return {
    float(cos(theta)), float(-sin(theta)), 0, 0,
    float(sin(theta)), float(cos(theta)),  0, 0,
    0,                 0,                  1, 0,
    0,                 0,                  0, 1
  };
}

inline Mat4 m4_perspective(float fov, float aspect, float near, float far) {
  float height = cos(fov) / sin(fov);
  float width = height / aspect;
  float f_range = far / (far - near);
  return {
    width, 0, 0, 0,
    0, height, 0, 0,
    0, 0, f_range, 1.0f,
    0, 0, -f_range * near, 0
  };
}

static Mat4 m4_rotate_x(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f,    c,    s, 0.0f,
    0.0f,   -s,    c, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
}

static Mat4 m4_rotate_y(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return {
       c, 0.0f,   -s, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
       s, 0.0f,    c, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
}

static Mat4 m4_rotate_z(float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return {
       c,    s, 0.0f, 0.0f,
      -s,    c, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
}

inline Mat4 m4_lookat(Vec3 center, Vec3 eye, Vec3 up) {
  Vec3 z = v3_normalize(center - eye);
  Vec3 x = v3_normalize(v3_cross(up, z));
  Vec3 y = v3_cross(z, x);

  return {
    x.x,              y.x,             z.x,            0,
    x.y,              y.y,             z.y,            0,
    x.z,              y.z,             z.z,            0,
    v3_dot(x, eye * -1), v3_dot(y, eye * -1), v3_dot(z, eye * -1), 1
  };
}