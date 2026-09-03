#include "SystemTime.h"

namespace phx
{
    void SystemTime::Initialize()
    {
        platform::SystemTime::Initialize();
    }

    int64_t SystemTime::GetCurrentTick()
    {
        return platform::SystemTime::GetCurrentTick();
    }

    void SystemTime::BusyLoopSleep(float sleepTime)
    {
        const int64_t final_tick = static_cast<int64_t>(static_cast<double>(sleepTime) / platform::SystemTime::GetTickDelta()) + GetCurrentTick();
        while (GetCurrentTick() < final_tick);
    }
}
