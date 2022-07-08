#define WASM_EXPORT __attribute__((visibility("default")))

extern void putchar(char);

WASM_EXPORT void init(void) {
  putchar('!');
}
