#define WASM_EXPORT __attribute__((visibility("default"))) extern "C"
#define WASM_IMPORT extern "C"


struct Rect {
  int x, y, w, h;
};

extern "C" void putrect(Rect);

Rect r;

WASM_EXPORT void frame(void) {
  putrect(r);
}

WASM_EXPORT void init(void) {
  r = {32, 32, 32, 32};
}
