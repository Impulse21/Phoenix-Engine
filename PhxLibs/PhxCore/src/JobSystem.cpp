#include "PhxCore_pch.h"

#include <PhxCore/Memory/FrameMemoryManager.h>
#include <PhxCore/JobSystem.h>
#include <PhxCore/ThreadContext.h>
#include <PhxCore/EnumUtils.h>
#include <PhxCore/RingBuffer.h>
#include <PhxCore/SystemTime.h>

#include <thread>
#include <algorithm>
#include <condition_variable>

#ifdef PHX_PLATFORM_WINDOWS
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
		JobSystem::Type Type = JobSystem::Type::Generic;
		size_t FrameId = ~0u;
		JobContext Context;

		void Execute()
		{
			Task(Context);
			KickoffThreadBarrier->Signal();
		}
	};

	using JobQueue = ThreadSafeRingBuffer<Job, 256>;

	thread_local size_t g_worker_last_frame_id = std::numeric_limits<size_t>::max();
	thread_local uint32_t g_worker_thread_id = std::numeric_limits<uint32_t>::max();

	struct ThreadPoolContext
	{
		JobSystem::Type CtxType;
		uint32_t NumThreads = 0;
		std::vector<std::thread> WorkerThreads;
		EnumArray<std::unique_ptr<JobQueue[]>, JobSystem::Priority> JobQueuePerThread;
		std::condition_variable WakeCondition;
		std::mutex WakeMutex;
		std::atomic_uint32_t NextQueue = 0;

		void DoWork(size_t threadId)
		{
			DoWork(JobSystem::Priority::High, threadId);

			if (CtxType == JobSystem::Type::Generic)
				DoWork(JobSystem::Priority::Low, threadId);
		}

		void DoWork(JobSystem::Priority prio, size_t threadId)
		{
			Job job;
			for (size_t i = 0; i < NumThreads; i++)
			{
				JobQueue& jobQueue = JobQueuePerThread[prio][threadId % NumThreads];
				while (jobQueue.Pop(job))
				{
					if (job.Type != JobSystem::Type::Streaming)
					{
						if (g_worker_last_frame_id == std::numeric_limits<size_t>::max() ||
							job.FrameId > g_worker_last_frame_id)
						{
							BeginFrame(); // Reset your linear/stack allocator here
							g_worker_last_frame_id = job.FrameId;
						}
					}

					job.Execute();
				}
				threadId++;
			}
		}

		void BeginFrame()
		{
			FrameMemoryManager::ResetCurrentThreadFrameAreana();
		}
	};


	std::atomic_bool g_is_alive = false;
	phx::EnumArray<ThreadPoolContext, JobSystem::Type> g_thread_pools;
	thread_local phx::EnumArray<JobSystem::Barrier, JobSystem::Type> g_thread_barrier;

	// use R
	struct Shutdowner
	{
		~Shutdowner()
		{
			JobSystem::Shutdown();
		}
	} g_shutdowner;

	void SubmitJobInternal(JobSystem::JobCallbackFunc const& task, JobSystem::Priority prio, JobSystem::Type type, JobContext* specifiedCtx)
	{
		JobContext context = {
			.FrameHeap = specifiedCtx ? specifiedCtx->FrameHeap : &FrameMemoryManager::GetCurrentThreadArena(),
		};
		ThreadPoolContext& ctx = g_thread_pools[type];
		if (ctx.NumThreads < 1)
		{
			task(context);
			return;
		}

		// const size_t frame_id = EngineSync::g_frame_count;
		const size_t frame_id = 0;
		Job job = {
			.Task = task,
			.KickoffThreadBarrier = &g_thread_barrier[type],
			.FrameId = frame_id,
			.Context = context
		};

		job.KickoffThreadBarrier->Add();

		ctx.JobQueuePerThread[prio][ctx.NextQueue.fetch_add(1) % ctx.NumThreads].Push(job);
		ctx.WakeCondition.notify_one();
	};
}

uint32_t phx::ThreadContext::GetWorkerThreadId()
{
	return g_worker_thread_id;
}

void JobSystem::Initialize()
{
	const uint32_t numCores = (uint32_t)GetNumCores();
	g_is_alive.store(true);

	uint32_t global_rhi_thread_counter = 0;
	CpuTimer timer;
	for (size_t i = 0; i < g_thread_pools.size(); i++)
	{
		Type type = static_cast<Type>(i);
		ThreadPoolContext& resource = g_thread_pools[i];

		switch (type)
		{
		case JobSystem::Type::Generic:
			resource.NumThreads = numCores - 1; // -1 for main thread;
			break;
		case JobSystem::Type::Streaming:
			resource.NumThreads = 1;
			break;
		default:
			PHX_ASSERT(false, "Unsupported type hit");
			break;
		}

		resource.CtxType = type;
		resource.NumThreads = std::max(1u, std::min(resource.NumThreads, numCores));
		resource.JobQueuePerThread[JobSystem::Priority::High] = std::make_unique<JobQueue[]>(resource.NumThreads);

		if (type == Type::Generic)
			resource.JobQueuePerThread[JobSystem::Priority::Low] = std::make_unique<JobQueue[]>(resource.NumThreads);

		resource.WorkerThreads.reserve(resource.NumThreads);

		for (uint32_t threadID = 0; threadID < resource.NumThreads; threadID++)
		{
			uint32_t global_rhi_thread_index = global_rhi_thread_counter++;
			std::thread& worker = resource.WorkerThreads.emplace_back([threadID, global_rhi_thread_index, &resource] {
				g_worker_thread_id = global_rhi_thread_index;
				FrameMemoryManager::EnsureThreadFrameArenaInitialized();
				while (g_is_alive)
				{
					resource.DoWork(threadID);

					std::unique_lock<std::mutex> lock(resource.WakeMutex);
					resource.WakeCondition.wait(lock);
				}
				FrameMemoryManager::ShutdownCurrentThreadFrameArena();
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

			if (type == Type::Generic)
			{
				BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
				assert(priorityResult != 0);

				std::wstringstream wss;
				wss << "[PHX] TP_Generic_" << threadID;
				HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
				assert(SUCCEEDED(hr));
			}
			else if (type == Type::Streaming)
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
		g_thread_pools[Type::Generic].NumThreads,
		g_thread_pools[Type::Streaming].NumThreads);
}

void JobSystem::Shutdown()
{
	if (!g_is_alive)
		return;

	g_is_alive.store(false);
	bool wakeLoop = true;

	std::thread waker([&]
		{
			while (wakeLoop)
			{
				for (auto& res : g_thread_pools)
					res.WakeCondition.notify_all();
			}
		});

	for (auto& res : g_thread_pools)
	{
		for (auto& thread : res.WorkerThreads)
			thread.join();
	}

	wakeLoop = false;
	waker.join();

	for (auto& thread_pool : g_thread_pools)
	{
		thread_pool.NumThreads = 0;
		thread_pool.WorkerThreads.clear();
		for (auto& ctx : thread_pool.JobQueuePerThread)
			ctx.reset();
		thread_pool.NextQueue = 0;
	}
}


void JobSystem::SubmitJob(JobCallbackFunc const& task, Priority priority, JobContext* specifiedCtx)
{
	SubmitJobInternal(task, priority, Type::Generic, specifiedCtx);
}

void JobSystem::SubmitJobToStreaming(JobCallbackFunc const& task, JobContext* specifiedCtx)
{
	SubmitJobInternal(task, Priority::High, Type::Streaming, specifiedCtx);
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
	return g_thread_barrier[type].IsNotCleared();
}

void JobSystem::Wait(Type type)
{
	if (IsBusy(type))
	{
		ThreadPoolContext& ctx = g_thread_pools[type];
		ctx.WakeCondition.notify_all();
		ctx.DoWork(ctx.NextQueue.fetch_add(1) % ctx.NumThreads);

		while (IsBusy(type))
		{
			std::this_thread::yield();
		}
	}
}

void phx::JobSystem::Flush()
{
	JobSystem::Wait(Type::Generic);
	JobSystem::Wait(Type::Streaming);
}

void JobSystem::Wait(Barrier& barrier, Type type)
{
	// Not sure I want to add here.
	// barrier.Add();

	while (barrier.IsNotCleared())
	{
		ThreadPoolContext& ctx = g_thread_pools[type];
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
	ThreadPoolContext& ctx = g_thread_pools[type];
	ctx.WakeCondition.notify_one();
}

uint32_t JobSystem::GetThreadCount(Type type)
{
	return g_thread_pools[type].NumThreads;
}

uint32_t phx::JobSystem::GetNumCores()
{
	return std::thread::hardware_concurrency();
}

