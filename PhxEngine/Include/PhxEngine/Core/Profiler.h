#pragma once

#include <array>
#include <vector>
#include <unordered_map>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/StringHash.h>

// TODO: REmove
namespace PhxEngine::RHI
{
	IGraphicsDevice;

}
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

	using RangeId = uint32_t;
	class FrameProfiler
	{
	public:
		FrameProfiler(RHI::IGraphicsDevice* gfxDevice);
		FrameProfiler(RHI::IGraphicsDevice* graphicsDevice, size_t numBackBuffers);

		void BeginFrame();
		void EndFrame();

		RangeId BeginRangeCPU(std::string const& name);
		void EndRangeCPU(RangeId id);

		RangeId BeginRangeGPU(std::string const& name, RHI::ICommandList* cmd);
		void EndRangeGPU(RangeId id);

		void BuildUI();

	private:
		struct Range
		{
			std::string Name;
			StatHistory Stats;
			bool IsActive;
			RHI::ICommandList* Cmd = nullptr;
			StopWatch CpuTimer;
			std::vector<RHI::TimerQueryHandle> TimerQueries;

			void Free(RHI::IGraphicsDevice* gfxDevice)
			{
				if (this->Cmd)
				{
					for (auto& timerQuery : TimerQueries)
					{
						gfxDevice->DeleteTimerQuery(timerQuery);
					}
					TimerQueries.clear();
				}
			}
		};

	private:
		std::mutex m_mutex;

		// -- Buffer per frame ---
		std::unordered_map<RangeId, Range> m_profileRanges;
		size_t m_frameCounter = 0;

		size_t m_cpuRangeId;
		size_t m_gpuRangeId;
	};
}

