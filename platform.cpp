#include "platform.h"

#define INTERN_STACK_SIZE 1 << 12
static u8 intern_stack[INTERN_STACK_SIZE];
static usize intern_stack_len = 0;

PLATFORM_EXPORT void* getstack(usize n) {
    if ((n + intern_stack_len) >= INTERN_STACK_SIZE) {
        return nullptr;
    }
    void *ret = intern_stack + intern_stack_len;
    intern_stack_len += n;
    return ret;
}

PLATFORM_EXPORT void setstack(void* at) {
    if (at < intern_stack || at >= (intern_stack+intern_stack_len)) {
        // TODO: Log invalid location
    }

    intern_stack_len = (u8*)at-intern_stack;
}
