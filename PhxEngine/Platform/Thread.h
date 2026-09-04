#pragma once

#include <thread>

namespace phx::platform::Thread
{
    // -- Thread stuff ---
    enum class Priority
    {
        High = 0,
        Normal,
        Low,
    };


    void SetThreadName(std::thread& thread, const char* name);
    void SetThreadAffinity(std::thread& thread, int affinity);
    void SetThreadPriority(std::thread& thread, Priority prio);
}