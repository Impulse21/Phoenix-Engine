#pragma once

#include <array>
#include <vector>
#include "Core/StopWatch.h"
#include "Graphics/RHI/PhxRHI.h"

namespace PhxEngine::Core
{
	class StatHistory
	{
	public:
		StatHistory();

		void Record(size_t frameNum, float value);

		float GetLast() const { return this->m_recent; }
		float GetAvg() const { return this->m_average; }
		float GetMin() const { return this->m_min; }
		float GetMax() const { return this->m_max; }

		const float* GetHistory() const { return this->m_history.data(); }
		const size_t GetHistorySize() const { return this->m_history.size(); }

		const float* GetExtendedHistory() const { return this->m_extendedHistory.data(); }
		const size_t GetExentedHistorySize() const { return this->m_extendedHistory.size(); }

	private:
		static const uint32_t kHistorySize = 64;
		static const uint32_t kExtendedHistorySize = 256;

		std::array<float, kHistorySize> m_history;
		std::array<float, kExtendedHistorySize> m_extendedHistory;

		float m_recent;
		float m_average;
		float m_min;
		float m_max;
	};

	class FrameProfiler
	{
	public:
		FrameProfiler(RHI::IGraphicsDevice* graphicsDevice, size_t numBackBuffers);

		void BeginFrame(RHI::CommandListHandle cmd);
		void EndFrame(RHI::CommandListHandle cmd);

		const StatHistory& GetCpuTimeStats() const { return this->m_cpuTime; }
		const StatHistory& GetGpuTimeStats() const { return this->m_gpuTime; }

	private:
		RHI::TimerQueryHandle& GetCurrentFrameGpuQuery() { return this->m_timerQueries[this->m_frameCounter % this->m_timerQueries.size()]; }

	private:
		RHI::IGraphicsDevice* m_graphicsDevice;

		// -- Buffer per frame ---
		std::vector<RHI::TimerQueryHandle> m_timerQueries;

		size_t m_frameCounter = 0;

		StopWatch m_cpuTimer;
		StatHistory m_cpuTime;
		StatHistory m_gpuTime;
		StatHistory m_frameDelta;
	};
}

