#include <PhxEngine/Platform/SystemTime.h>

#include <time.h>

using namespace phx::platform;

namespace
{
    double g_tick_delta = 0.0;
}

void SystemTime::Initialize()
{
    // clock_gettime(CLOCK_MONOTONIC) reports whole seconds + nanoseconds
    // rather than a raw counter/frequency pair, so ticks are just nanoseconds.
    g_tick_delta = 1.0 / 1000000000.0;
}

int64_t SystemTime::GetCurrentTick()
{
    struct timespec ts;

    // CLOCK_MONOTONIC is the direct equivalent to QueryPerformanceCounter.
    // It measures uptime and is not affected by system time-of-day changes.
    const int result = clock_gettime(CLOCK_MONOTONIC, &ts);
    PHX_ASSERT(result == 0 && "Unable to query monotonic clock");

    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + static_cast<int64_t>(ts.tv_nsec);
}

double SystemTime::GetTickDelta()
{
    return g_tick_delta;
}
