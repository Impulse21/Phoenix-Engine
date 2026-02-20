#include "PhxCore_pch.h"

#include <PhxCore/Memory/FrameMemoryManager.h>
#include <PhxCore/TaskScheduler.h>
#include <PhxCore/ThreadContext.h>
#include <PhxCore/EnumUtils.h>
#include <PhxCore/RingBuffer.h>
#include <PhxCore/SystemTime.h>

#include <thread>
#include <algorithm>
#include <condition_variable>

#if defined(PHX_PLATFORM_WINDOWS)
#include <Windows.h>
#include <sstream>
#include <PHX_ASSERT.h>
#elif defined(PHX_PLATFORM_LINUX)
#include <pthread.h>
#endif

using namespace phx;

namespace
{
	struct Task
	{
		TaskScheduler::TaskCallbackFunc task;
		TaskScheduler::Barrier* kickoff_barrier = nullptr;

		void Execute()
		{
			Task();
			kickoff_barrier->Signal();
		}
	};

	using TaskQueue = ThreadSafeRingBuffer<Task, 256>;

	thread_local uint32_t g_worker_thread_id = std::numeric_limits<uint32_t>::max();

	struct ThreadPool
	{
		std::string name;
		uint32_t num_threads = 0;
		bool has_low_queue = false;
		
		std::vector<std::thread> worker_threads;

		std::unique_ptr<TaskQueue[]> high_queues;
		std::unique_ptr<TaskQueue[]> low_queues;
		
		std::condition_variable wake_condidition;
		std::mutex wake_mutex;
		std::atomic_uint32_t next_queue = 0;

		TaskScheduler::Barrier pool_barrier;

		void DoWork(size_t start_thread)
		{
			DoWorkOnQueues(high_queues.get(), start_thread);

			if (has_low_queue && low_queues)
				DoWorkOnQueues(low_queues.get(), start_thread);
		}

	private:
		void DoWorkOnQueues(TaskQueue* task_queues, size_t start_thread)
		{
			Task task;
			for (size_t i = 0; i < num_threads; i++)
			{
				TaskQueue& task_queue = task_queues[start_thread % num_threads];
				while (task_queue.Pop(task))
				{
					task.Execute();
				}

				start_thread++;
			}
		}

		void BeginFrame()
		{
			FrameMemoryManager::ResetCurrentThreadFrameAreana();
		}
	};


	constexpr uint32_t k_max_pools = 16;

	std::atomic_bool g_is_alive = false;
	std::mutex g_registry_mutex;
	std::array<std::unique_ptr<ThreadPool>, k_max_pools> g_pools = {};

	uint32_t g_pool_count = 0;
	uint32_t g_global_thread_counter = 0;
	
    // Per-calling-thread barriers, one per registered pool slot.
    // Workers only need a single PoolBarrier on the pool itself (see above).
	thread_local std::array<TaskScheduler::Barrier, k_max_pools> g_thread_barrier;


    ThreadPoolHandle g_generic_handle = {};

	// use R
	struct Shutdowner
	{
		~Shutdowner()
		{
			TaskScheduler::Shutdown();
		}
	} g_shutdowner;

    void StartPoolThreads(ThreadPool* pool, int os_priority, uint32_t num_cores)
    {
        pool->worker_threads.reserve(pool->num_threads);

        for (uint32_t thread_id = 0; thread_id < pool->num_threads; thread_id++)
        {
            uint32_t global_index = g_global_thread_counter++;

            std::thread& worker = pool->num_threads.emplace_back([thread_id, global_index, pool]
            {
                g_worker_thread_id = global_index;
                FrameMemoryManager::EnsureThreadFrameArenaInitialized();

                while (g_is_alive)
                {
                    pool->DoWork(thread_id);

                    std::unique_lock<std::mutex> lock(pool->wake_mutex);
                    pool->wake_condidition.wait(lock);
                }

                FrameMemoryManager::ShutdownCurrentThreadFrameArena();
            });

#if defined(PHX_PLATFORM_WINDOWS)
            HANDLE handle = (HANDLE)worker.native_handle();

            int core = (int)(threadID + 1);
            DWORD_PTR affinityMask   = 1ull << (core % numCores);
            DWORD_PTR affinityResult = SetThreadAffinityMask(handle, affinityMask);
            PHX_ASSERT(affinityResult > 0);

            BOOL priorityResult = SetThreadPriority(handle, osPriority);
            PHX_ASSERT(priorityResult != 0);

            std::wstringstream wss;
            wss << L"[PHX] " << pool->Name.c_str() << L"_" << threadID;
            HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
            PHX_ASSERT(SUCCEEDED(hr));
#elif defined(PHX_PLATFORM_LINUX)
			pthread_t handle = worker.native_handle();

			// 2. Set Thread Affinity
			int core = (int)(thread_id + 1);
			cpu_set_t cpu_set;
			CPU_ZERO(&cpu_set);
			CPU_SET(core % num_cores, &cpu_set);

			int affinityResult = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpu_set);
			PHX_ASSERT(affinityResult == 0); // In POSIX, 0 indicates success

			// 3. Set Thread Priority (Scheduling Policy)
			// Note: POSIX priority requires a scheduling policy (e.g., SCHED_RR or SCHED_OTHER)
			struct sched_param param;
			param.sched_priority = osPriority; 
			int priorityResult = pthread_setschedparam(handle, SCHED_OTHER, &param);
			PHX_ASSERT(priorityResult == 0);

			// 4. Set Thread Name
			// Linux limits thread names to 16 characters (including null terminator)
			std::string threadName = "PHX_" + std::to_string(threadID);
			int nameResult = pthread_setname_np(handle, threadName.substr(0, 15).c_str());
			PHX_ASSERT(nameResult == 0);
#endif
        }
    }

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
			PHX_PHX_ASSERT(false, "Unsupported type hit");
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
			PHX_ASSERT(affinityResult > 0);

			if (type == Type::Generic)
			{
				BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
				PHX_ASSERT(priorityResult != 0);

				std::wstringstream wss;
				wss << "[PHX] TP_Generic_" << threadID;
				HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
				PHX_ASSERT(SUCCEEDED(hr));
			}
			else if (type == Type::Streaming)
			{
				BOOL priorityResult = SetThreadPriority(handle, THREAD_PRIORITY_LOWEST);
				PHX_ASSERT(priorityResult != 0);

				std::wstringstream wss;
				wss << "[PHX] TP_Streaming_" << threadID;
				HRESULT hr = SetThreadDescription(handle, wss.str().c_str());
				PHX_ASSERT(SUCCEEDED(hr));
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

