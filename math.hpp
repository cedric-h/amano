#include "platform.h"

#define cos(x) __builtin_cos(x)
#define sin(x) __builtin_sin(x)

inline Mat4 operator*(Mat4 me, Mat4 they) {
  Mat4 ret = {};

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      float dot = 0;
      for (int k = 0; k < 4; ++k) {
        dot += me.num[i][k] * they.num[k][j];
      }
      ret.num[i][j] = dot;
    }
  }

  return ret;
}

inline Mat4 operator*=(Mat4 &me, Mat4 they) {
  return me = me * they;
}

inline Mat4 math_m4_scale(Vec3 scale)
{
  return {
    scale.x, 0,       0,       0,
    0,       scale.y, 0,       0,
    0,       0,       scale.z, 0,
    0,       0,       0,       1
  };
}

inline Mat4 math_m4_identity() {
  return {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
}

inline Mat4 math_m4_rotation2d(float theta) {
  return {
    float(cos(theta)), float(-sin(theta)), 0, 0,
    float(sin(theta)), float(cos(theta)),  0, 0,
    0,                 0,                  1, 0,
    0,                 0,                  0, 1
  };
}