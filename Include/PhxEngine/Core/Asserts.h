#pragma once

// Disable assertions by commenting out the below line.
#define PHX_ASSERTIONS_ENABLED

#ifdef PHX_ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

#define PHX_ASSERT(expr)                                             \
    {                                                                \
        if (expr) {                                                  \
        } else {                                                     \
                                                                     \
            debugBreak();                                            \
        }                                                            \
    }

#else
#define PHX_ASSERT(expr)
#endif