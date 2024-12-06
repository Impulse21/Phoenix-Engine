#include "ThreadPool.h"

#include "phxRingBuffer.h"
#include <thread>
#include <algorithm>
#include <condition_variable>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <sstream>
#include <assert.h>

#endif
using namespace phx;

namespace
{
	using JobQueue = ThreadSafeRingBuffer<std::function<void()>, 256>;

	uint32_t m_numThreads = 0;
	std::vector<std::thread> m_workerThreads;
	std::unique_ptr<JobQueue[]> m_jobQueuePerThread;
	std::condition_variable m_wakeCondition;
	std::mutex m_wakeMutex;
	uint64_t m_currentFenceValue;
	std::atomic_uint64_t m_finishedFenceValue;
	std::atomic_bool m_alive = false;
	std::atomic_uint32_t m_nextQueue =  0 ;

	void Poll()
	{
		m_wakeCondition.notify_one();
		std::this_thread::yield(); // allow this thread to be rescheduled;
	}

	// use R
	struct Shutdowner
	{
		~Shutdowner()
		{
			ThreadPool::Finalize();
		}
	} m_shutdowner;


	// Ran per thread - steals jobs.
	void ThreadDoWork(size_t threadId)
	{
		std::function<void()> job;
		for (size_t i = 0; i < m_numThreads; i++)
		{
			JobQueue& jobQueue = m_jobQueuePerThread[threadId % m_numThreads];
			while (jobQueue.Pop(job))
			{
				job();
				m_finishedFenceValue.fetch_add(1);
			}
			threadId++;
		}
	}
}

void ThreadPool::Initialize()
{
	m_finishedFenceValue.store(0ull);
	m_currentFenceValue = 0;

	const uint32_t numCores = std::thread::hardware_concurrency() - 1; // -1 for main thread.
	m_numThreads = std::max(1u, numCores);

	m_jobQueuePerThread.reset(new JobQueue[m_numThreads]);
	for (uint32_t threadID = 0; threadID < m_numThreads; threadID++)
	{
		std::thread& worker = m_workerThreads.emplace_back([threadID] {
			while(m_alive)
			{
				ThreadDoWork(threadID);
				
				std::unique_lock<std::mutex> lock(m_wakeMutex);
				m_wakeCondition.wait(lock);
			}
		});

		// TODO:
#ifdef _WIN32
		HANDLE handle = (HANDLE)worker.native_handle();
		DWORD_PTR affinityMask = 1ull << threadID;
		DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
		assert(affinityResult > 0);

		BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
		assert(priorityResult != 0);

		std::wstringstream wss;
		wss << "[PHX] ThreadPool_" << threadID;
		HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
		assert(SUCCEEDED(hr));
#endif
	}

	m_alive.store(true);
}

void ThreadPool::Finalize()
{
	if (!m_alive)
		return;

	m_alive.store(false);
	bool wakeLoop = true;

	std::thread waker([&]
	{
		while(wakeLoop)
		{
			m_wakeCondition.notify_all();
		}
	});

	for (auto& thread : m_workerThreads)
		thread.join();

	wakeLoop = false;
	waker.join();

	m_workerThreads.clear();
	m_numThreads = 0;
	m_jobQueuePerThread.reset();
	m_nextQueue = 0;
}

void ThreadPool::Submit(std::function<void()> const& job)
{
	// Main Thread state
	m_currentFenceValue += 1;

	if ( m_numThreads == 1)
	{
		job();
		return;
	}

	m_jobQueuePerThread[m_nextQueue.fetch_add(1) % m_numThreads].Push(job);
	m_wakeCondition.notify_one();
}

#if false
void ThreadPool::Dispatch(uint32_t jobCount, uint32_t groupSize, std::function<void(JobDispatchArgs)> const& job)
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
#endif

bool ThreadPool::IsBusy()
{
	return m_finishedFenceValue.load() < m_currentFenceValue;
}

void ThreadPool::Wait()
{
	while(IsBusy()) { Poll(); }
}


void ThreadPool::Wait(Barrier& barrier)
{
	barrier.Counter++;
	
	while (barrier.Counter.load() > 0)
	{
		m_wakeCondition.notify_all();
		ThreadDoWork(m_nextQueue.fetch_add(1) % m_numThreads);

		while (barrier.Counter.load() > 0)
			std::this_thread::yield();
	}
}

void ThreadPool::Signal(Barrier& barrier)
{
	barrier.Counter--;
	m_wakeCondition.notify_one();
}

size_t ThreadPool::GetThreadCount()
{
	return m_numThreads;
}

