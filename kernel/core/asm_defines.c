#include "../thread_kernel.h"
#include "../../userspace/core/core.h"
#include "core_kernel.h"

#include <stdint.h>

#define DEFINE(sym, val) asm volatile("\n-> " #sym " %0 " #val "\n" : : "i" (val))
#define OFFSETOF(s, m) \
    DEFINE(offsetof_##s##_##m, offsetof(s, m));

#define SIZEOF(s) \
    DEFINE(sizeof_##s, sizeof(s));

void foo()
{
    OFFSETOF(THREAD, sp);
    SIZEOF(GLOBAL);
    OFFSETOF(KERNEL, active_thread);
    OFFSETOF(KERNEL, next_thread);
    SIZEOF(KERNEL);
}
