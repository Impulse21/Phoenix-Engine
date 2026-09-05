#pragma once

#if defined(PHX_PROFILING_ENABLED)
    #include <tracy/Tracy.hpp>

    #include <cstring>

    #define PHX_PROFILE_FRAME()          FrameMark
    #define PHX_PROFILE_SCOPE()          ZoneScoped
    #define PHX_PROFILE_SCOPE_N(name)    ZoneScopedN(name)
    #define PHX_PROFILE_PLOT(name, val)  TracyPlot(name, val)
    #define PHX_PROFILE_MSG(text)        TracyMessage(text, strlen(text))

    #define PHX_PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
    #define PHX_PROFILE_FREE(ptr)        TracyFree(ptr)

    #define PHX_PROFILE_LOCK(var, mtx)   TracyLockable(std::mutex, var)
#else
    #define PHX_PROFILE_FRAME()
    #define PHX_PROFILE_SCOPE()
    #define PHX_PROFILE_SCOPE_N(name)
    #define PHX_PROFILE_PLOT(name, val)
    #define PHX_PROFILE_MSG(text)

    #define PHX_PROFILE_ALLOC(ptr, size)
    #define PHX_PROFILE_FREE(ptr)

    #define PHX_PROFILE_LOCK(var, mtx)   mtx var
#endif
