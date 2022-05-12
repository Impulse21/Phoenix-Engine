#pragma once

#include <chrono>
#include "TimeStep.h"

namespace PhxEngine::Core
{
	class StopWatch
	{
	public:
		// Record a reference timestamp
		inline void Begin()
		{
			this->m_timestamp = std::chrono::high_resolution_clock::now();
		}

		// Elapsed time in milliseconds since the wi::Timer creation or last call to record()
		inline TimeStep Elapsed()
		{
			auto timestamp2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(timestamp2 - this->m_timestamp);
			return timeSpan.count();
		}

	private:
		std::chrono::high_resolution_clock::time_point m_timestamp = std::chrono::high_resolution_clock::now();
	};
}