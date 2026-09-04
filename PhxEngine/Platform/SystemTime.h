#pragma once

#include <stdint.h>

namespace phx::platform::SystemTime
{
    // Query the performance counter frequency. Must be called once before
    // GetCurrentTick() is used for anything time-based.
    void Initialize();

    // Query the current value of the performance counter.
    int64_t GetCurrentTick();

    // Seconds per tick, valid after Initialize().
    double GetTickDelta();
}
