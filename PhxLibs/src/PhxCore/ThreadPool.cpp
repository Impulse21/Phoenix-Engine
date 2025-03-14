#include "PhxCore_pch.h"

#include "PhxCore/ThreadPool.h"
#include "PhxCore/EnumUtils.h"
#include "PhxCore/RingBuffer.h"
#include "PhxCore/SystemTime.h"

#include <thread>
#include <algorithm>
#include <condition_variable>

#ifdef _WIN32
#include <Windows.h>
#include <sstream>
#include <assert.h>
#endif

using namespace phx;

namespace
{
	struct Job
	{
		std::function<void()> Task;
		ThreadPool::Barrier* KickoffThreadBarrier = nullptr;

		void Execute()
		{
			Task();
			KickoffThreadBarrier->Signal();
		}
	};

	using JobQueue = ThreadSafeRingBuffer<Job, 256>;

	struct ThreadPoolContext
	{
		uint32_t NumThreads = 0;
		std::vector<std::thread> WorkerThreads;
		std::unique_ptr<JobQueue[]> JobQueuePerThread;
		std::condition_variable WakeCondition;
		std::mutex WakeMutex;
		std::atomic_uint32_t NextQueue = 0;

		void DoWork(size_t threadId)
		{
			Job job;
			for (size_t i = 0; i < NumThreads; i++)
			{
				JobQueue& jobQueue = JobQueuePerThread[threadId % NumThreads];
				while (jobQueue.Pop(job))
				{
					job.Execute();
				}
				threadId++;
			}
		}
	};
	
	thread_local phx::EnumArray<ThreadPool::Barrier, ThreadPool::Type> m_threadBarrier;
	std::atomic_bool m_alive = false;
	phx::EnumArray<ThreadPoolContext, ThreadPool::Type> m_threadPools;

	// use R
	struct Shutdowner
	{
		~Shutdowner()
		{
			ThreadPool::Finalize();
		}
	} m_shutdowner;
	
}

void ThreadPool::Initialize()
{
	const uint32_t numCores = (uint32_t)GetNumCores();
	m_alive.store(true);

	CpuTimer timer;
	for (size_t i = 0; i < m_threadPools.size(); i++)
	{
		Type type = static_cast<Type>(i);
		ThreadPoolContext& resource = m_threadPools[i];

		switch (type)
		{
		case ThreadPool::Type::High:
			resource.NumThreads = numCores - 1; // -1 for main thread;
			break;
		case ThreadPool::Type::Streaming:
			resource.NumThreads = 1;
			break;
		default:
			PHX_ASSERT(false, "Unsupported type hit");
			break;
		}

		resource.NumThreads = std::max(1u, std::min(resource.NumThreads, numCores));
		resource.JobQueuePerThread.reset(new JobQueue[resource.NumThreads]);
		resource.WorkerThreads.reserve(static_cast<size_t>(resource.NumThreads));

		for (uint32_t threadID = 0; threadID < resource.NumThreads; threadID++)
		{
			std::thread& worker = resource.WorkerThreads.emplace_back([threadID, &resource] {
				while (m_alive)
				{
					resource.DoWork(threadID);

					std::unique_lock<std::mutex> lock(resource.WakeMutex);
					resource.WakeCondition.wait(lock);
				}
			});
			
#ifdef _WIN32
			HANDLE handle = (HANDLE)worker.native_handle();
			int core = threadID + 1; // put threads on increasing cores starting from 2nd
			if (type== Type::Streaming)
			{
				// Put streaming to last core:
				core = numCores - 1 - threadID;
			}

			DWORD_PTR affinityMask = 1ull << core;
			DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
			assert(affinityResult > 0);

			if (type == Type::High)
			{
				BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
				assert(priorityResult != 0);

				std::wstringstream wss;
				wss << "[PHX] TP_High_" << threadID;
				HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
				assert(SUCCEEDED(hr));
			}
			else if (type == Type::High)
			{
				BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_LOWEST);
				assert(priorityResult != 0);

				std::wstringstream wss;
				wss << "[PHX] TP_Streaming_" << threadID;
				HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
				assert(SUCCEEDED(hr));
			}
#endif
		}
	}

	CpuTimeStep duration = timer.Elapsed();
	PHX_CORE_INFO("[ThreadPool] Initialized with {0} cores in {1} ms\n\tHigh priority threads: {2}\n\tStreaming threads: {3}",
		numCores,
		duration.GetMilliseconds(),
		m_threadPools[Type::High].NumThreads,
		m_threadPools[Type::Streaming].NumThreads);
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
			for (auto& res : m_threadPools)
				res.WakeCondition.notify_all();
		}
	});

	for (auto& res : m_threadPools)
	{
		for (auto& thread : res.WorkerThreads)
			thread.join();
	}

	wakeLoop = false;
	waker.join();
	
	for (auto& res : m_threadPools)
	{
		res.WorkerThreads.clear();
		res.NumThreads = 0;
		res.JobQueuePerThread.reset();
		res.NextQueue = 0;
	}
}

void ThreadPool::SubmitTask(std::function<void()> const& task, Type type)
{
	ThreadPoolContext& ctx = m_threadPools[type];
	if (ctx.NumThreads < 1)
	{
		task();
		return;
	}

	Job job = {
		.Task = task,
		.KickoffThreadBarrier = &m_threadBarrier[type]
	};

	job.KickoffThreadBarrier->Add();
	
	ctx.JobQueuePerThread[ctx.NextQueue.fetch_add(1) % ctx.NumThreads].Push(job);
	ctx.WakeCondition.notify_one();
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

bool ThreadPool::IsBusy(Type type)
{
	return m_threadBarrier[type].IsNotCleared();
}

void ThreadPool::Wait(Type type)
{
	if (IsBusy(type))
	{
		ThreadPoolContext& ctx = m_threadPools[type];
		ctx.WakeCondition.notify_all();
		ctx.DoWork(ctx.NextQueue.fetch_add(1) % ctx.NumThreads);

		while (IsBusy(type))
		{
			std::this_thread::yield();
		}
	}
}

void ThreadPool::Wait(Barrier& barrier, Type type)
{
	// Not sure I want to add here.
	// barrier.Add();
	
	while (barrier.IsNotCleared())
	{
		ThreadPoolContext& ctx = m_threadPools[type];
		ctx.WakeCondition.notify_all();
		ctx.DoWork(ctx.NextQueue.fetch_add(1) % ctx.NumThreads);

		while (barrier.IsNotCleared())
		{
			std::this_thread::yield();
		}
	}
}

void ThreadPool::Signal(Barrier& barrier, Type type)
{
	barrier.Signal();
	ThreadPoolContext& ctx = m_threadPools[type];
	ctx.WakeCondition.notify_one();
}

uint32_t ThreadPool::GetThreadCount(Type type)
{
	return m_threadPools[type].NumThreads;
}

uint32_t phx::ThreadPool::GetNumCores()
{
	return std::thread::hardware_concurrency();
}

