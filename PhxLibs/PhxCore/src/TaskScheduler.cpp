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
	struct Task
	{
		TaskScheduler::TaskCallbackFunc task;
		TaskScheduler::Barrier* kickoff_barrier = nullptr;

		void Execute()
		{
			task();
			kickoff_barrier->Signal();
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
                    pool->wake_condidition.wait(lock, [pool]{
						return !g_is_alive || pool->pool_barrier.IsNotCleared();;
					});
                }

                FrameMemoryManager::ShutdownCurrentThreadFrameArena();
            });

			int core = (int)(thread_id + 1);
			phx::Platform::SetThreadAffinity(worker, (core % num_cores));
			phx::Platform::SetThreadPriority(worker, os_priority);

			const std::string thread_name = "PHX_" + pool->name + std::to_string(thread_id);
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

    queues[pool->next_queue.fetch_add(1) % pool->num_threads].Push(task);
    pool->wake_condidition.notify_one();
}

namespace
{
	bool IsBusy(ThreadPoolImpl* pool)
	{
		return pool->pool_barrier.IsNotCleared();
	}

	void Wait(ThreadPoolImpl* pool)
	{
		if (!IsBusy(pool))
			return;

		pool->wake_condidition.notify_all();
		pool->DoWork(pool->next_queue.fetch_add(1) % pool->num_threads);

		while (IsBusy(pool))
			std::this_thread::yield();
	}
}
bool TaskScheduler::IsBusy(ThreadPoolHandle handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle");

    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	return pool->pool_barrier.IsNotCleared();
}

void TaskScheduler::Wait(ThreadPoolHandle handle)
{
    PHX_ASSERT(
		g_thread_pool_registry.Contains(handle),
		"Invalid ThreadPoolHandle");

    ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
	Wait(pool);
}

void TaskScheduler::Wait(Barrier& barrier, ThreadPoolHandle handle)
{
    while (barrier.IsNotCleared())
    {
    	ThreadPoolImpl* pool = g_thread_pool_registry.Get(handle);
        pool->wake_condidition.notify_all();
        pool->DoWork(pool->next_queue.fetch_add(1) % pool->num_threads);

        while (barrier.IsNotCleared())
            std::this_thread::yield();
    }
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
		Wait(&pool);
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

