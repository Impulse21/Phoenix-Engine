#include "PhxEngine/PhxEngine_pch.h"
#include "JobSystem.h"
#include "PhxCore/EnumUtils.h"
#include "PhxCore/RingBuffer.h"
#include "PhxCore/SystemTime.h"
#include <PhxCore/Memory/MemorySystem.h>

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
		JobSystem::JobCallbackFunc Task;
		JobSystem::Barrier* KickoffThreadBarrier = nullptr;
		JobSystem::Type Type = JobSystem::Type::High;
		size_t FrameId = ~0u;
		JobContext Context;

		void Execute()
		{
			Task(Context);
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
					if (job.Type != JobSystem::Type::Streaming)
					{
						const size_t currFrameId = phx::EngineSync::g_FrameCount;
						if (job.FrameId > currFrameId)
							BeginFrame();
					}

					job.Execute();
				}
				threadId++;
			}
		}

		void BeginFrame()
		{
			MemorySystem::ResetCurrentThreadFrameAreana();
		}
	};

	thread_local phx::EnumArray<JobSystem::Barrier, JobSystem::Type> m_threadBarrier;
	std::atomic_bool m_alive = false;
	phx::EnumArray<ThreadPoolContext*, JobSystem::Type> m_threadPools;

	// use R
	struct Shutdowner
	{
		~Shutdowner()
		{
			JobSystem::Shutdown();
		}
	} m_shutdowner;

}

void JobSystem::Initialize()
{
	const uint32_t numCores = (uint32_t)GetNumCores();
	m_alive.store(true);

	CpuTimer timer;
	for (size_t i = 0; i < m_threadPools.size(); i++)
	{
		Type type = static_cast<Type>(i);
		m_threadPools[i] = phx_new ThreadPoolContext();
		ThreadPoolContext& resource = *m_threadPools[i];

		switch (type)
		{
		case JobSystem::Type::High:
			resource.NumThreads = numCores - 1; // -1 for main thread;
			break;
		case JobSystem::Type::Streaming:
			resource.NumThreads = 1;
			break;
		default:
			PHX_ASSERT(false, "Unsupported type hit");
			break;
		}

		resource.NumThreads = std::max(1u, std::min(resource.NumThreads, numCores));
		resource.JobQueuePerThread = std::make_unique<JobQueue[]>(resource.NumThreads);
		resource.WorkerThreads.reserve(resource.NumThreads);

		for (uint32_t threadID = 0; threadID < resource.NumThreads; threadID++)
		{
			std::thread& worker = resource.WorkerThreads.emplace_back([threadID, &resource] {
				MemorySystem::EnsureThreadFrameArenaInitialized();
				while (m_alive)
				{
					resource.DoWork(threadID);

					std::unique_lock<std::mutex> lock(resource.WakeMutex);
					resource.WakeCondition.wait(lock);
				}
				MemorySystem::ShutdownCurrentThreadFrameArena();
			});

#ifdef _WIN32
			HANDLE handle = (HANDLE)worker.native_handle();
			int core = threadID + 1; // put threads on increasing cores starting from 2nd
			if (type == Type::Streaming)
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
		m_threadPools[Type::High]->NumThreads,
		m_threadPools[Type::Streaming]->NumThreads);
}

void JobSystem::Shutdown()
{
	if (!m_alive)
		return;

	m_alive.store(false);
	bool wakeLoop = true;

	std::thread waker([&]
		{
			while (wakeLoop)
			{
				for (auto& res : m_threadPools)
					res->WakeCondition.notify_all();
			}
		});

	for (auto& res : m_threadPools)
	{
		for (auto& thread : res->WorkerThreads)
			thread.join();
	}

	wakeLoop = false;
	waker.join();

	for (size_t i = 0; i < m_threadPools.size(); i++)
	{
		phx_delete m_threadPools[i];
		m_threadPools[i] = nullptr;
	}
}

void JobSystem::SubmitJob(JobCallbackFunc const& task, Type type, JobContext* specifiedCtx)
{
	JobContext context = {
		.FrameHeap = specifiedCtx ? specifiedCtx->FrameHeap : &MemorySystem::GetCurrentThreadArena(),
	};
	ThreadPoolContext& ctx = *m_threadPools[type];
	if (ctx.NumThreads < 1)
	{
		task(context);
		return;
	}

	const size_t frameId = EngineSync::g_FrameCount;
	Job job = {
		.Task = task,
		.KickoffThreadBarrier = &m_threadBarrier[type],
		.FrameId = frameId,
		.Context = context
	};

	job.KickoffThreadBarrier->Add();

	ctx.JobQueuePerThread[ctx.NextQueue.fetch_add(1) % ctx.NumThreads].Push(job);
	ctx.WakeCondition.notify_one();
}

#if false
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
#endif

bool JobSystem::IsBusy(Type type)
{
	return m_threadBarrier[type].IsNotCleared();
}

void JobSystem::Wait(Type type)
{
	if (IsBusy(type))
	{
		ThreadPoolContext& ctx = *m_threadPools[type];
		ctx.WakeCondition.notify_all();
		ctx.DoWork(ctx.NextQueue.fetch_add(1) % ctx.NumThreads);

		while (IsBusy(type))
		{
			std::this_thread::yield();
		}
	}
}

void JobSystem::Wait(Barrier& barrier, Type type)
{
	// Not sure I want to add here.
	// barrier.Add();

	while (barrier.IsNotCleared())
	{
		ThreadPoolContext& ctx = *m_threadPools[type];
		ctx.WakeCondition.notify_all();
		ctx.DoWork(ctx.NextQueue.fetch_add(1) % ctx.NumThreads);

		while (barrier.IsNotCleared())
		{
			std::this_thread::yield();
		}
	}
}

void JobSystem::Signal(Barrier& barrier, Type type)
{
	barrier.Signal();
	ThreadPoolContext& ctx = *m_threadPools[type];
	ctx.WakeCondition.notify_one();
}

uint32_t JobSystem::GetThreadCount(Type type)
{
	return m_threadPools[type]->NumThreads;
}

uint32_t phx::JobSystem::GetNumCores()
{
	return std::thread::hardware_concurrency();
}

