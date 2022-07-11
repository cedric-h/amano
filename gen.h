#include "platform.h"

struct Ascii8x8Font {
    u8 data[128][8];
};

// Constant: font8x8_basic
// Contains an 8x8 font map for unicode points U+0000 - U+007F (basic latin)
extern const Ascii8x8Font gen_font8x8_basic;