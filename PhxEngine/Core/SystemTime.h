#pragma once

#include <PhxEngine/Platform/SystemTime.h>

#include <stdint.h>

namespace phx::SystemTime
{
    // Must be called once (see Engine::Initialize) before any of the
    // Ticks-to-* conversions below are meaningful.
    void Initialize();

    // Query the current value of the performance counter.
    int64_t GetCurrentTick();

    void BusyLoopSleep(float sleepTime);

    inline float TicksToSeconds(int64_t tickCount);
    inline double TicksToMillisecs(int64_t tickCount);
    inline double TicksToNanosecs(int64_t tickCount);
    inline double TimeBetweenTicks(int64_t tick1, int64_t tick2);
}

namespace phx
{
    class CpuTimeStep
    {
    public:
        CpuTimeStep(int64_t ticks = 0ll)
            : m_ticks(ticks)
        {
        }

        operator int64_t() const { return this->m_ticks; }

        double GetSeconds() const { return SystemTime::TicksToSeconds(m_ticks); }
        double GetMilliseconds() const { return SystemTime::TicksToMillisecs(m_ticks); }
        double GetNanoseconds() const { return SystemTime::TicksToNanosecs(m_ticks); }

    private:
        int64_t m_ticks = 0ul;
    };

    class CpuTimer
    {
    public:
        CpuTimer()
         : m_timestamp(0ll)
        {
            Begin();
        };

        // Record a reference timestamp
        inline void Begin()
        {
            this->m_timestamp = SystemTime::GetCurrentTick();
        }

        inline void Reset()
        {
            this->m_timestamp = SystemTime::GetCurrentTick();
        }

        // Elapsed time in milliseconds since the wi::Timer creation or last call to record()
        inline CpuTimeStep Elapsed()
        {
            auto timestamp2 = SystemTime::GetCurrentTick();
            return CpuTimeStep(timestamp2 - m_timestamp);
        }

    private:
        int64_t m_timestamp;
    };
}

namespace phx::SystemTime
{
    inline float TicksToSeconds(int64_t tickCount)
    {
        return static_cast<float>(tickCount) * platform::SystemTime::GetTickDelta();
    }

    inline double TicksToMillisecs(int64_t tickCount)
    {
        return static_cast<double>(tickCount) * platform::SystemTime::GetTickDelta() * 1000.0;
    }

    inline double TicksToNanosecs(int64_t tickCount)
    {
        return static_cast<double>(tickCount) * platform::SystemTime::GetTickDelta() * 1e9;
    }

    inline double TimeBetweenTicks(int64_t tick1, int64_t tick2)
    {
        return TicksToSeconds(tick2 - tick1);
    }
}
