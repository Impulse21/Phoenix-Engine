#include "PhxCore_pch.h"

#include <PhxCore/Memory/FrameMemoryManager.h>
#include <PhxCore/TaskScheduler.h>
#include <PhxCore/ThreadContext.h>
#include <PhxCore/EnumUtils.h>
#include <PhxCore/RingBuffer.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Platform/Platform.h>

#include <thread>
#include <algorithm>
#include <condition_variable>

using namespace phx;

namespace phx
{
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
}

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

            std::thread& worker = pool->worker_threads.emplace_back([thread_id, global_index, pool]
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

			int core = (int)(thread_id + 1);
			phx::Platform::SetThreadAffinity(worker, (core % num_cores));
			phx::Platform::SetThreadPriority(worker, os_priority);

			const std::string thread_name = "PHX_" + std::to_string(thread_id);
			phx::Platform::SetThreadName(worker, thread_name.c_str());
        }
    }
}

uint32_t phx::ThreadContext::GetWorkerThreadId()
{
	return g_worker_thread_id;
}

void TaskScheduler::Initialize()
{
	g_is_alive = true;
}

ThreadPoolHandle phx::TaskScheduler::CreateThreadPool(const ThreadPoolDescriptor& desc)
{
	const uint32_t num_cores = GetNumCores();

	auto thread_pool = std::make_unique<ThreadPool>();
	thread_pool->name = desc.name;
	thread_pool->num_threads = desc.num_threads == 0
		? std::max(1u, num_cores - 1u)
		: std::max(1u, std::min(desc.num_threads, num_cores));

	thread_pool->has_low_queue = desc.has_low_queue;

	ThreadPoolHandle handle(g_pool_count, 1);
	g_pools[g_pool_count++] = std::move(thread_pool);
#if false
	pool->HighQueues = std::make_unique<TaskQueue[]>(pool->NumThreads);
	if (pool->HasLowQueue)
		pool->LowQueues = std::make_unique<TaskQueue[]>(pool->NumThreads);

	ThreadPoolHandle handle;
	{
		std::lock_guard<std::mutex> lock(g_registry_mutex);
		PHX_ASSERT(g_pool_count < kMaxPools, "Too many thread pools registered");
		handle.Index = g_pool_count;
		g_pools[g_pool_count++] = pool;
	}

#ifdef PHX_PLATFORM_WINDOWS
	int osPriority = desc.OSPriority != 0 ? desc.OSPriority : THREAD_PRIORITY_NORMAL;
#else
	int osPriority = desc.OSPriority;
#endif

	StartPoolThreads(pool, osPriority, numCores);

	PHX_CORE_INFO("[TaskScheduler] Registered pool '{}' with {} threads (LowQueue={})",
		pool->Name, pool->NumThreads, pool->HasLowQueue);
#endif

	return handle;
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

