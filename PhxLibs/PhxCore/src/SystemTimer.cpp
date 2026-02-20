#include "PhxCore_pch.h"

#include "PhxCore/SystemTime.h"
#include <assert.h>

using namespace phx;


double SystemTime::sm_CpuTickDelta = 0.0;

// Query the performance counter frequency
void SystemTime::Initialize()
{
#if defined(PHX_PLATFORM_WINDOWS)
	LARGE_INTEGER frequency;
	assert(TRUE == QueryPerformanceFrequency(&frequency) && "Unable to query performance counter frequency");
	sm_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
#elif defined(Phx_PLATFORM_LINUX)
	sm_CpuTickDelta = 1.0 / 1000000000.0;
#endif
}

// Query the current value of the performance counter
int64_t SystemTime::GetCurrentTick()
{

#if defined(PHX_PLATFORM_WINDOWS)
	LARGE_INTEGER currentTick;
	assert(TRUE == QueryPerformanceCounter(&currentTick) && "Unable to query performance counter value");
	return static_cast<int64_t>(currentTick.QuadPart);
#elif defined(PHX_PLATFORM_LINUX)
	struct timespec ts;
    // CLOCK_MONOTONIC is the direct equivalent to QPC. 
    // It measures uptime and is not affected by system time-of-day changes.
    int result = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(result == 0 && "Unable to query monotonic clock");

    // Convert to total nanoseconds to return a single tick value
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + static_cast<int64_t>(ts.tv_nsec);
#endif
}

void SystemTime::BusyLoopSleep(float sleepTime)
{
	int64_t finalTick = (int64_t)((double)sleepTime / sm_CpuTickDelta) + GetCurrentTick();
	while (GetCurrentTick() < finalTick);
}
