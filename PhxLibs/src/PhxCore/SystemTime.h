#pragma once

#include <stdint.h>

namespace phx
{
	class SystemTime
	{
	public:

		// Query the performance counter frequency
		static void Initialize();

		// Query the current value of the performance counter
		static int64_t GetCurrentTick();

		static void BusyLoopSleep(float sleepTime);

		static inline double TicksToSeconds(int64_t tickCount)
		{
			return static_cast<double>(tickCount) * sm_CpuTickDelta;
		}

		static inline double TicksToMillisecs(int64_t tickCount)
		{
			return static_cast<double>(tickCount) * sm_CpuTickDelta * 1000.0;
		}

		static inline double TicksToNanosecs(int64_t tickCount)
		{
			return static_cast<double>(tickCount) * sm_CpuTickDelta * 1000.0;
		}

		static inline double TimeBetweenTicks(int64_t tick1, int64_t tick2)
		{
			return TicksToSeconds(tick2 - tick1);
		}

	private:
		static double sm_CpuTickDelta;
	};

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
		double GetNanoseconds() const { return SystemTime::TicksToMillisecs(m_ticks); }

	private:
		int64_t m_ticks = 0.0f;
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