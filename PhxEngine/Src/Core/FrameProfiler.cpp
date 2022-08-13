#include "phxpch.h"
#include "PhxEngine/Core/FrameProfiler.h"

using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

PhxEngine::Core::StatHistory::StatHistory()
	: m_average(0.0f)
	, m_recent(0.0f)
	, m_min(0.0f)
	, m_max(0.0f)
	, m_history({})
	, m_extendedHistory({})
{
}

void PhxEngine::Core::StatHistory::Record(size_t frameNum, float value)
{
	this->m_history[frameNum % kHistorySize] = value;
	this->m_extendedHistory[frameNum % kExtendedHistorySize] = value;
	this->m_recent = value;

	size_t validCount = 0;
	this->m_min = FLT_MAX;
	this->m_max = 0;

	for (float v : this->m_history)
	{
		if (v > 0.0f)
		{
			validCount++;
			this->m_average += v;
			this->m_min = std::min(v, this->m_min);
			this->m_max = std::max(v, this->m_max);
		}
	}

	if (validCount > 0)
	{
		this->m_average /= (float)validCount;
	}
	else
	{
		this->m_min = 0.0f;
	}
}

PhxEngine::Core::FrameProfiler::FrameProfiler(RHI::IGraphicsDevice* graphicsDevice, size_t numBackBuffers)
	: m_graphicsDevice(graphicsDevice)
{
	this->m_timerQueries.resize(numBackBuffers);
	for (int i = 0; i < numBackBuffers; i++)
	{
		this->m_timerQueries[i] = this->m_graphicsDevice->CreateTimerQuery();
	}
}

void PhxEngine::Core::FrameProfiler::BeginFrame(RHI::CommandListHandle cmd)
{
	this->m_cpuTimer.Begin();

	RHI::TimerQueryHandle& query = this->GetCurrentFrameGpuQuery();
	cmd->BeginTimerQuery(query);

}

void PhxEngine::Core::FrameProfiler::EndFrame(RHI::CommandListHandle cmd)
{
	auto cpuElapsedTime = this->m_cpuTimer.Elapsed();
	this->m_cpuTime.Record(this->m_frameCounter, cpuElapsedTime.GetMilliseconds());

	RHI::TimerQueryHandle& query = this->GetCurrentFrameGpuQuery();
	cmd->EndTimerQuery(query);

	TimeStep timer = this->m_graphicsDevice->GetTimerQueryTime(query);

	this->m_gpuTime.Record(this->m_frameCounter, timer.GetMilliseconds());

	this->m_frameCounter++;
}
