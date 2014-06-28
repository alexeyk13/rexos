#include "../svc_process.h"
#include "../../userspace/core/core.h"
#include "../kernel.h"

#include <stdint.h>

#define DEFINE(sym, val) asm volatile("\n-> " #sym " %0 " #val "\n" : : "i" (val))
#define OFFSETOF(s, m) \
    DEFINE(offsetof_##s##_##m, offsetof(s, m));

#define SIZEOF(s) \
    DEFINE(sizeof_##s, sizeof(s));

void foo()
{
    OFFSETOF(PROCESS, sp);
    SIZEOF(GLOBAL);
    OFFSETOF(KERNEL, active_process);
    OFFSETOF(KERNEL, next_process);
    SIZEOF(KERNEL);
}
