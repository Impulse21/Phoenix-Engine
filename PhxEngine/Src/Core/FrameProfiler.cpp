#include "phxpch.h"
#include "PhxEngine/Core/FrameProfiler.h"
#include "imgui.h"
#include <PhxEngine/Core/StringHash.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;

#define DISABLED

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
PhxEngine::Core::FrameProfiler::FrameProfiler(RHI::IGraphicsDevice* gfxDevice)
	: m_gfxDevice(gfxDevice)
{
}

PhxEngine::Core::FrameProfiler::FrameProfiler(RHI::IGraphicsDevice* graphicsDevice, size_t numBackBuffers)
	: m_gfxDevice(graphicsDevice)
{
}

void PhxEngine::Core::FrameProfiler::BeginFrame()
{
#ifdef DISABLED
	return;
#endif
	this->m_cpuRangeId = this->BeginRangeCPU("CPU Frame");

	RHI::ICommandList* cmd = this->m_gfxDevice->BeginCommandRecording();
	this->m_gpuRangeId = this->BeginRangeGPU("GPU Frame", cmd);
	cmd->Close();
	this->m_gfxDevice->ExecuteCommandLists({ cmd });
}

void PhxEngine::Core::FrameProfiler::EndFrame()
{
#ifdef DISABLED
	return;
#endif
	this->EndRangeCPU(this->m_cpuRangeId);

	RHI::ICommandList* cmd = this->m_gfxDevice->BeginCommandRecording();
	this->m_profileRanges[this->m_gpuRangeId].Cmd = cmd;
	this->EndRangeGPU(this->m_gpuRangeId);

	cmd->Close();
	this->m_gfxDevice->ExecuteCommandLists({ cmd });

	this->m_frameCounter++;
}

RangeId PhxEngine::Core::FrameProfiler::BeginRangeCPU(std::string const& name)
{
#ifdef DISABLED
	return 0;
#endif
	Core::StringHash hash(name);

	auto _ = std::scoped_lock(this->m_mutex);
	assert(!this->m_profileRanges[hash].IsActive);

	this->m_profileRanges[hash].Name = name;
	this->m_profileRanges[hash].Cmd = nullptr;
	this->m_profileRanges[hash].IsActive = true;
	this->m_profileRanges[hash].CpuTimer.Begin();

	return hash;
}

void PhxEngine::Core::FrameProfiler::EndRangeCPU(RangeId id)
{
#ifdef DISABLED
	return;
#endif
	assert(this->m_profileRanges[id].IsActive);
	assert(!this->m_profileRanges[id].Cmd);

	auto _ = std::scoped_lock(this->m_mutex);
	TimeStep elapsedTime = this->m_profileRanges[id].CpuTimer.Elapsed();
	this->m_profileRanges[id].Stats.Record(this->m_frameCounter, elapsedTime.GetMilliseconds());
	this->m_profileRanges[id].IsActive = false;
}

RangeId PhxEngine::Core::FrameProfiler::BeginRangeGPU(std::string const& name, RHI::ICommandList* cmd)
{
#ifdef DISABLED
	return 0;
#endif
	Core::StringHash hash(name);

	auto _ = std::scoped_lock(this->m_mutex);
	assert(!this->m_profileRanges[hash].IsActive);

	this->m_profileRanges[hash].Name = name;
	this->m_profileRanges[hash].Cmd = cmd;
	if (this->m_profileRanges[hash].TimerQueries.empty())
	{
		this->m_profileRanges[hash].TimerQueries.resize(this->m_gfxDevice->GetViewportDesc().BufferCount);
		for (int i = 0; i < this->m_profileRanges[hash].TimerQueries.size(); i++)
		{
			this->m_profileRanges[hash].TimerQueries[i] = this->m_gfxDevice->CreateTimerQuery();
		}
	}

	TimerQueryHandle currentQuery = this->m_profileRanges[hash].TimerQueries[this->m_frameCounter % this->m_profileRanges[hash].TimerQueries.size()];
	cmd->BeginTimerQuery(currentQuery);
	this->m_profileRanges[hash].IsActive = true;

	return hash;
}

void PhxEngine::Core::FrameProfiler::EndRangeGPU(RangeId id)
{
#ifdef DISABLED
	return;
#endif
	assert(this->m_profileRanges[id].IsActive);
	assert(this->m_profileRanges[id].Cmd);

	auto _ = std::scoped_lock(this->m_mutex);
	TimerQueryHandle currentQuery = this->m_profileRanges[id].TimerQueries[this->m_frameCounter % this->m_profileRanges[id].TimerQueries.size()];
	this->m_profileRanges[id].Cmd->EndTimerQuery(currentQuery);

	TimeStep timer = this->m_gfxDevice->GetTimerQueryTime(currentQuery);

	this->m_profileRanges[id].Stats.Record(this->m_frameCounter, timer.GetMilliseconds());
	this->m_profileRanges[id].IsActive = false;
}

void PhxEngine::Core::FrameProfiler::BuildUI()
{
	ImGui::Text("Profile:");
	ImGui::Indent();
	ImGui::Text(
		"GPU Frame Time: %5.3f ms",
		this->m_profileRanges[this->m_gpuRangeId].Stats.GetAvg());
	ImGui::Indent();

	for (auto& itr : this->m_profileRanges)
	{
		if (itr.first == this->m_gpuRangeId)
		{
			continue;
		}
		if (itr.second.Cmd)
		{
			ImGui::Text(
				"%s: %5.2f ms",
				itr.second.Name.c_str(),
				itr.second.Stats.GetAvg());
		}
	}
	ImGui::Unindent();

	ImGui::Separator();

	ImGui::Text(
		"CPU Frame Time: %5.3f ms",
		this->m_profileRanges[this->m_cpuRangeId].Stats.GetAvg());
		ImGui::Indent();

		for (auto& itr : this->m_profileRanges)
		{
			if (itr.first == this->m_cpuRangeId)
			{
				continue;
			}

			if (!itr.second.Cmd)
			{
				ImGui::Text(
					"%s: %5.2f ms",
					itr.second.Name.c_str(),
					itr.second.Stats.GetAvg());
			}
		}
		ImGui::Unindent();
		ImGui::Unindent();
}
