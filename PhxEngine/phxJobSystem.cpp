#include "pch.h"

#include "phxJobSystem.h"
#include "phxPlatformDetection.h"

#include "phxRingBuffer.h"
#include <thread>
#include <algorithm>
#include <condition_variable>

#ifdef PHX_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <sstream>
#include <assert.h>
#endif

using namespace phx;

namespace
{
	uint32_t m_numThreads = 0;
	ThreadSafeRingBuffer<std::function<void()>, 256> m_jobPool;
	std::condition_variable m_wakeCondition;
	std::mutex m_wakeMutex;
	uint64_t m_currentFenceValue;
	std::atomic_uint64_t m_finishedFenceValue;

	void Poll()
	{
		m_wakeCondition.notify_one();
		std::this_thread::yield(); // allow this thread to be rescheduled;
	}

}

void JobSystem::Initialize()
{
	m_finishedFenceValue.store(0ull);
	m_currentFenceValue = 0;

	uint32_t numCores = std::thread::hardware_concurrency();
	m_numThreads = std::max(1u, numCores);

	for (uint32_t threadID = 0; threadID < m_numThreads; threadID++)
	{
		std::thread worker([]
		{
			std::function<void()> currentJob;

			while(true)
			{
				if (m_jobPool.Pop(currentJob))
				{
					currentJob();
					m_finishedFenceValue.fetch_add(1);
				}
				else
				{
					std::unique_lock<std::mutex> lock(m_wakeMutex);
					m_wakeCondition.wait(lock);
				}
			}
		});

		// TODO:
#ifdef PHX_PLATFORM_WINDOWS
		HANDLE handle = (HANDLE)worker.native_handle();
		DWORD_PTR affinityMask = 1ull << threadID;
		DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
		assert(affinityResult > 0);

		BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
		assert(priorityResult != 0);

		std::wstringstream wss;
		wss << "phxJobSystem_" << threadID;
		HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
		assert(SUCCEEDED(hr));
#endif
		worker.detach();
	}
}

void JobSystem::Execute(std::function<void()> const& job)
{
	// Main Thread state
	m_currentFenceValue += 1;

	while (!m_jobPool.Push(job)) { Poll(); }

	m_wakeCondition.notify_one();
}

void JobSystem::Dispatch(uint32_t jobCount, uint32_t groupSize, std::function<void(JobDispatchArgs)> const& job)
{
	if (jobCount == 0 || groupSize == 0)
		return;

	const uint32_t groupCount = (jobCount + groupSize - 1) / groupSize;

	// The main thread label state is updated:
	m_currentFenceValue += groupCount;

	for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
	{
		// For each group, generate one real job:
		auto jobGroup = [jobCount, groupSize, job, groupIndex]() {

			// Calculate the current group's offset into the jobs:
			const uint32_t groupJobOffset = groupIndex * groupSize;
			const uint32_t groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);

			JobDispatchArgs args;
			args.GroupIndex = groupIndex;

			// Inside the group, loop through all job indices and execute job for each index:
			for (uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
			{
				args.JobIndex = i;
				job(args);
			}
		};

		// Try to push a new job until it is pushed successfully:
		while (!m_jobPool.Push(jobGroup)) { Poll(); }

		m_wakeCondition.notify_one(); // wake one thread
	}
}

bool JobSystem::IsBusy()
{
	return m_finishedFenceValue.load() < m_currentFenceValue;
}

void JobSystem::Wait()
{
	while(IsBusy()) { Poll(); }
}
