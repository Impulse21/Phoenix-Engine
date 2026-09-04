#include <PhxEngine/Platform/SystemTime.h>

#include <windows.h>

using namespace phx::platform;

namespace
{
    double g_tick_delta = 0.0;
}

void SystemTime::Initialize()
{
    LARGE_INTEGER frequency;
    PHX_ASSERT(QueryPerformanceFrequency(&frequency) && "Unable to query performance counter frequency");
    g_tick_delta = 1.0 / static_cast<double>(frequency.QuadPart);
}

int64_t SystemTime::GetCurrentTick()
{
    LARGE_INTEGER current_tick;
    PHX_ASSERT(QueryPerformanceCounter(&current_tick) && "Unable to query performance counter value");
    return static_cast<int64_t>(current_tick.QuadPart);
}

double SystemTime::GetTickDelta()
{
    return g_tick_delta;
}
