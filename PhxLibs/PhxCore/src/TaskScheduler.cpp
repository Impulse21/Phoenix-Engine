#include "PhxCore_pch.h"

#include <PhxCore/Memory/FrameMemoryManager.h>
#include <PhxCore/TaskScheduler.h>
#include <PhxCore/ThreadContext.h>
#include <PhxCore/EnumUtils.h>
#include <PhxCore/RingBuffer.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/Platform/Platform.h>
#include <PhxCore/Pool.h>

#include <thread>
#include <algorithm>
#include <condition_variable>

using namespace phx;

namespace
{
	struct BarrierSignaller
	{
    	TaskScheduler::Barrier* barrier;
    	~BarrierSignaller() { barrier->Signal(); }
	};

	struct Task
	{
		TaskScheduler::TaskCallbackFunc task;
		TaskScheduler::Barrier* kickoff_barrier = nullptr;

		void Execute()
		{
			BarrierSignaller signaller{ kickoff_barrier };
			task();
		}
	};

	using TaskQueue = ThreadSafeRingBuffer<Task, 256>;
	thread_local uint32_t g_worker_thread_id = std::numeric_limits<uint32_t>::max();

	struct alignas(64) ThreadPoolImpl
	{
		// -- Cold Data ---
		std::string name;
		uint32_t num_threads = 0;
		bool has_low_queue = false;
		std::vector<std::thread> worker_threads;

		// -- Hot Data ---
		std::atomic_uint32_t queued_tasks = 0;
		std::atomic_uint32_t next_queue = 0;
		std::condition_variable wake_condidition;
		std::mutex wake_mutex;

		// Per-thread queues - each padded to avoid false sharing between threads
		std::unique_ptr<TaskQueue[]> high_queues;
		std::unique_ptr<TaskQueue[]> low_queues;

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

				PHX_CORE_TRACE("[TaskScheduler] Thread {0} checking queue[{1}], empty={2}, barrier={3}, queued_tasks = {4}",
					g_worker_thread_id,
					start_thread % num_threads,
					task_queue.IsEmpty(),
					pool_barrier.Counter.load(),
					queued_tasks.load());

				while (task_queue.Pop(task))
				{
					queued_tasks.fetch_sub(1);
					PHX_CORE_TRACE("[TaskScheduler] Thread {0} popped task, barrier={1}, queued_tasks = {2}",
						g_worker_thread_id,
						pool_barrier.Counter.load(),
						queued_tasks.load());

					task.Execute();

					PHX_CORE_TRACE("[TaskScheduler] Thread {0} finished task, barrier={1}",
						g_worker_thread_id,
						pool_barrier.Counter.load());
				}

				start_thread++;
			}
		}

		void BeginFrame()
		{
			FrameMemoryManager::ResetCurrentThreadFrameAreana();
		}
	};

	std::atomic_bool g_is_alive = false;
	SmallObjectPool<ThreadPool, ThreadPoolImpl, 4> g_thread_pool_registry;

	uint32_t g_global_thread_counter = 0;
    ThreadPoolHandle g_core_handle = {};

	struct Shutdowner
	{
		~Shutdowner()
		{
			TaskScheduler::Shutdown();
		}
	} g_shutdowner;

	void DebugPrintPoolStatus(ThreadPoolImpl* pool)
	{
		PHX_CORE_WARN("[TaskScheduler] Pool '{0}' status:", pool->name);
		PHX_CORE_WARN("\tpending (barrier counter): {0}", pool->pool_barrier.Counter.load());
		PHX_CORE_WARN("\tnext_queue: {0}", pool->next_queue.load());
		
		// Check each queue depth
		for (uint32_t i = 0; i < pool->num_threads; i++)
		{
			PHX_CORE_WARN("\thigh_queue[{0}] size: {1}", i, pool->high_queues[i].Size());
			if (pool->has_low_queue && pool->low_queues)
				PHX_CORE_WARN("\tlow_queue[{0}] size: {1}", i, pool->low_queues[i].Size());
		}	
	}

    void StartPoolThreads(ThreadPoolImpl* pool, Platform::ThreadPriority os_priority, uint32_t num_cores)
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
                    pool->wake_condidition.wait(lock, [pool]
					{
						return !g_is_alive || pool->queued_tasks.load() > 0;
					});
                }

                FrameMemoryManager::ShutdownCurrentThreadFrameArena();
            });

			int core = (int)(thread_id + 1);
			phx::Platform::SetThreadAffinity(worker, (core % num_cores));
			phx::Platform::SetThreadPriority(worker, os_priority);

			const std::string thread_name = "PHX_" + pool->name + "_" + std::to_string(thread_id);
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

	// Register main thread
	g_worker_thread_id = g_global_thread_counter++;
}

ThreadPoolHandle phx::TaskScheduler::CreateThreadPool(const ThreadPoolDescriptor& desc)
{
	const uint32_t num_cores = GetNumCores();

	ThreadPoolHandle handle = g_thread_pool_registry.Allocate();

	ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	pool->name = desc.name;
	pool->num_threads = desc.num_threads == 0
		? std::max(1u, num_cores - 1u)
		: std::max(1u, std::min(desc.num_threads, num_cores));

	pool->has_low_queue = desc.has_low_queue;

	pool->high_queues = std::make_unique<TaskQueue[]>(pool->num_threads);
	if (pool->has_low_queue)
		pool->low_queues = std::make_unique<TaskQueue[]>(pool->num_threads);

	StartPoolThreads(pool, desc.os_priority, num_cores);

	PHX_CORE_INFO(
		"[TaskScheduler] Created pool '{0}' with {1} threads (LowQueue={2})",
		pool->name,
		pool->num_threads,
		pool->has_low_queue);

	return handle;
}

ThreadPoolHandle TaskScheduler::InitializeCorePool()
{
    CpuTimer timer;

    ThreadPoolDescriptor desc = {
		.name = "Core",
		.num_threads = 0,
		.os_priority = Platform::ThreadPriority::Normal,
		.has_low_queue = true
	};

    g_core_handle = CreateThreadPool(desc);

    CpuTimeStep duration = timer.Elapsed();
    PHX_CORE_INFO(
		"[TaskScheduler] Core pool ready in {0} ms ({1} threads)",
        duration.GetMilliseconds(),
        GetThreadCount(g_core_handle));

    return g_core_handle;
}

ThreadPoolHandle TaskScheduler::GetCorePool()
{
	return g_core_handle;
}

void TaskScheduler::Shutdown()
{
	if (!g_is_alive)
		return;

	g_is_alive.store(false);

	g_worker_thread_id = std::numeric_limits<uint32_t>::max();

	bool wakeLoop = true;
	std::thread waker([&] {
		while (wakeLoop)
		{
			g_thread_pool_registry.ForEach([](ThreadPoolImpl& pool) {
				pool.wake_condidition.notify_all();
			});
		}
	});

	
	g_thread_pool_registry.ForEach([](ThreadPoolImpl& pool) {
		for (auto& thread : pool.worker_threads)
			thread.join();
	});

	g_thread_pool_registry.Shutdown();
	wakeLoop = false;
	waker.join();
}


void phx::TaskScheduler::Dispatch(
	const DispatchCallbackFunc &task,
	uint32_t total_count,
	uint32_t group_size,
	ThreadPoolHandle handle,
	Priority priority)
{
	PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle passed to Submit");

	const uint32_t group_count = (total_count + group_size - 1) / group_size;
    for (uint32_t g = 0; g < group_count; ++g)
    {
        // Capture by value — each group task is self-contained
        Submit([task, g, group_size, group_count, total_count]()
        {
            const uint32_t start = g * group_size;
            const uint32_t end   = std::min(start + group_size, total_count);

            for (uint32_t local = 0; local < (end - start); ++local)
            {
				DispatchId dispatch_id = {
                    .global_index = start + local,
                    .group_index  = g,
                    .local_index  = local,
                    .group_count  = group_count,
                    .total_count  = total_count
                };

                task(dispatch_id);
            }
        },
        handle,
		priority);
    }
}

void TaskScheduler::Submit(
	TaskCallbackFunc const& callback,
	ThreadPoolHandle handle,
	Priority priority)
{
	PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle passed to Submit");
	
    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
    if (pool->num_threads < 1)
    {
        callback();
        return;
    }

    Task task = {
        .task = callback,
        .kickoff_barrier = &pool->pool_barrier
    };
	
    task.kickoff_barrier->Add();

    TaskQueue* queues = (priority == Priority::Low && pool->has_low_queue)
                        ? pool->low_queues.get()
                        : pool->high_queues.get();

	pool->queued_tasks.fetch_add(1);
    queues[pool->next_queue.fetch_add(1) % pool->num_threads].Push(task);
    pool->wake_condidition.notify_one();
}

namespace
{
	bool IsBusy(TaskScheduler::Barrier& barrier)
	{
		return barrier.IsNotCleared();
	}

	void Wait(ThreadPoolImpl* pool, TaskScheduler::Barrier& barrier)
	{
		if (!IsBusy(barrier))
			return;

		pool->DoWork(0);
		
		uint32_t spin_count = 0;
		while (IsBusy(barrier))
		{
			std::this_thread::yield();
			spin_count++;
			if (spin_count % 10000 == 0)
			{
				PHX_CORE_WARN("[TaskScheduler] Wait() spinning on pool '{0}', barrier={1}, spin={2}",
					pool->name,
					barrier.Counter.load(),
					spin_count);

				DebugPrintPoolStatus(pool); // or inline the dump here
			}
		}
	}
}
bool TaskScheduler::IsBusy(ThreadPoolHandle handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle");

    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	return ::IsBusy(pool->pool_barrier);
}

void TaskScheduler::Wait(ThreadPoolHandle handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle");

    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	Wait(pool, pool->pool_barrier);
}

void TaskScheduler::Wait(Barrier& barrier, ThreadPoolHandle handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle");

    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	Wait(pool, barrier);
}

void TaskScheduler::Signal(Barrier& barrier, ThreadPoolHandle handle)
{
	barrier.Signal();
    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	pool->wake_condidition.notify_one();
}

void TaskScheduler::Flush()
{
	g_thread_pool_registry.ForEach([](ThreadPoolImpl& pool) 
	{
		Wait(&pool, pool.pool_barrier);
	});
}

uint32_t TaskScheduler::GetThreadCount(ThreadPoolHandle handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle");


    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
    return pool->num_threads;
}

uint32_t phx::TaskScheduler::GetTotalThreadCount()
{
    return g_global_thread_counter;
}

uint32_t phx::TaskScheduler::GetNumCores()
{
	return std::thread::hardware_concurrency();
}

void phx::TaskScheduler::DebugPrintPoolStatus(ThreadPoolHandle pool_handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(pool_handle),
		"Invalid ThreadPoolHandle");
    ThreadPoolImpl* pool = g_thread_pool_registry.Get(pool_handle);
    DebugPrintPoolStatus(pool);
}