#include "JobSystem.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/RingBuffer.h>

#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

#include <thread>
#include <mutex>
#include <condition_variable>

using namespace phx;

PHX_CVAR_INT(jobs_queue_capacity, 256, "Per-thread job queue capacity");
PHX_CVAR_INT(jobs_max_pools,      4,   "Maximum number of thread pools");

namespace
{
    constexpr phx::Log::Channel k_log = { "Jobs" };

    struct Job
    {
        JobFn fn;
        Barrier* barrier = nullptr;

        void Execute()
        {
            fn();
            if (barrier)
                barrier->Signal();
        }
    };

    using JobQueue = RingBuffer<Job, 256>;

    thread_local u32 g_worker_thread_id = UINT32_MAX;
}

namespace phx
{
    struct PHX_CACHELINE_ALIGN ThreadPool
    {
        PHX_NO_COPY_NO_MOVE(ThreadPool);
        ThreadPool() = default;

        const char*     name          = "";
        u32             num_threads   = 0;
        bool            has_low_queue = false;
        IHeapAllocator* allocator     = nullptr;

        std::thread*    worker_threads = nullptr;

        PHX_CACHELINE_ALIGN std::atomic_uint32_t queued_jobs = 0;
        PHX_CACHELINE_ALIGN std::atomic_uint32_t next_queue   = 0;

        std::condition_variable wake_condition;
        std::mutex              wake_mutex;

        JobQueue* high_queues = nullptr;
        JobQueue* low_queues  = nullptr;
        
        Barrier pool_barrier;

        void DoWork(u32 start_thread)
        {
            DoWorkOnQueues(high_queues, start_thread);
            if (has_low_queue && low_queues)
                DoWorkOnQueues(low_queues, start_thread);
        }

    private:
        void DoWorkOnQueues(JobQueue* queues, u32 start_thread)
        {
            Job job;
            for (u32 i = 0; i < num_threads; i++)
            {
                JobQueue& q = queues[start_thread % num_threads];
                while (q.Pop(job))
                {
                    queued_jobs.fetch_sub(1, std::memory_order_relaxed);
                    job.Execute();
                }
                start_thread++;
            }
        }
    };   
}

// -- Internal state ---
namespace
{
    std::atomic_bool g_is_alive          = false;
    u32              g_global_thread_ctr = 0;
    IHeapAllocator* g_allocator        = nullptr;
    ThreadPool*     g_core_pool        = nullptr;

    constexpr u32 k_max_pools = 4;
    ThreadPoolHandle g_pools[k_max_pools] = {};
    u32 g_pool_count = 0;
    std::mutex g_pool_registry_mutex;


    void WaitOnBarrier(ThreadPool* pool, Barrier& barrier)
    {
        if (!barrier.IsBusy()) 
            return;

        pool->DoWork(0);

        u32 spin_count = 0;
        while (barrier.IsBusy())
        {
            std::this_thread::yield();
            spin_count++;

            if (spin_count % 10000 == 0)
            {
                PHX_LOG_WARN(k_log,
                    "Wait() spinning on pool '{}', barrier={}, spin={}",
                    pool->name,
                    barrier.counter.load(),
                    spin_count);

                phx::Jobs::DebugPrintPoolStatus(pool);
            }
        }
    }

    void StartPoolThreads(ThreadPool* pool, platform::Thread::Priority priority, u32 num_cores)
    {
        for (u32 t = 0; t < pool->num_threads; t++)
        {
            u32 global_idx = g_global_thread_ctr++;

            // Placement-new into pre-allocated storage
            new (&pool->worker_threads[t]) std::thread([t, global_idx, pool]()
            {
                g_worker_thread_id = global_idx;

                while (g_is_alive)
                {
                    pool->DoWork(t);

                    std::unique_lock lock(pool->wake_mutex);
                    pool->wake_condition.wait(lock, [pool]()
                    {
                        return !g_is_alive || pool->queued_jobs.load(std::memory_order_relaxed) > 0;
                    });
                }
            });

            // Affinity — skip core 0 (main thread), wrap around
            u32 core = (t + 1) % num_cores;
            phx::platform::Thread::SetThreadAffinity (pool->worker_threads[t], core);
            phx::platform::Thread::SetThreadPriority (pool->worker_threads[t], priority);

            char name_buf[64];
            snprintf(name_buf, sizeof(name_buf), "PHX_%s_%u", pool->name, t);
            phx::platform::Thread::SetThreadName(pool->worker_threads[t], name_buf);
        }
    }
}

// ── phx::Jobs implementation ──────────────────────────────────────────────────
namespace phx::Jobs
{
    void Initialize(IHeapAllocator* allocator)
    {
        PHX_ASSERT(allocator != nullptr);
        g_allocator = allocator;
        g_is_alive  = true;

        g_worker_thread_id = g_global_thread_ctr++;

        ThreadPoolDescriptor core_desc = {};
        core_desc.name         = "Core";
        core_desc.thread_count = 0;          // 0 = hardware_concurrency - 1
        core_desc.priority     = platform::Thread::Priority::Normal;
        core_desc.has_low_queue = true;

        g_core_pool = CreateThreadPool(core_desc);

        PHX_LOG_INFO(k_log,
            "Initialized — main thread id: {}, core pool threads: {}",
            g_worker_thread_id,
            g_core_pool->num_threads);
    }

    void Shutdown()
    {
        if (!g_is_alive) 
            return;

        g_is_alive.store(false, std::memory_order_seq_cst);
        g_worker_thread_id = UINT32_MAX;

        {
            std::scoped_lock _(g_pool_registry_mutex);
            for (u32 i = 0; i < g_pool_count; i++)
            {
                if (g_pools[i])
                    g_pools[i]->wake_condition.notify_all();
            }
        }

        // Join and destroy all pools
        for (u32 i = 0; i < g_pool_count; i++)
        {
            ThreadPool* pool = g_pools[i];
            if (!pool) continue;

            for (u32 t = 0; t < pool->num_threads; t++)
                pool->worker_threads[t].join();

            phx_delete_array(g_allocator, pool->worker_threads, pool->num_threads);
            phx_delete_array(g_allocator, pool->high_queues, pool->num_threads);
            if (pool->low_queues)
                phx_delete_array(g_allocator, pool->low_queues, pool->num_threads);

            phx_delete(*g_allocator, pool);
            g_pools[i] = nullptr;
        }

        g_pool_count = 0;
        g_core_pool  = nullptr;
        g_allocator  = nullptr;

        PHX_LOG_INFO(k_log, "Shutdown complete");
    }

    void BeginFrame()
    {
        // Hook for per-thread scratch allocator reset.
        // When per-thread scratch is added, each worker thread checks this
        // at the start of its next work loop iteration.
        // For now this is a no-op — placeholder for the memory integration.
    }

    ThreadPoolHandle CreateThreadPool(const ThreadPoolDescriptor& desc)
    {
        PHX_ASSERT(g_allocator  != nullptr);
        PHX_ASSERT(g_pool_count < k_max_pools);

        const u32 num_cores = GetNumCores();

        ThreadPool* pool    = phx_new(*g_allocator, ThreadPool);
        pool->allocator     = g_allocator;
        pool->name          = desc.name;
        pool->has_low_queue = desc.has_low_queue;
        pool->num_threads   = desc.thread_count == 0
            ? std::max(1u, num_cores - 1u)
            : std::max(1u, std::min(desc.thread_count, num_cores));

        pool->high_queues = phx_new_array(*g_allocator, TaskQueue, pool->num_threads);
        if (pool->has_low_queue)
            pool->low_queues = phx_new_array(*g_allocator, TaskQueue, pool->num_threads);

        pool->worker_threads = static_cast<std::thread*>(
            g_allocator->Alloc(sizeof(std::thread) * pool->num_threads, alignof(std::thread)));

        StartPoolThreads(pool, desc.priority, num_cores);

        {
            std::scoped_lock _(g_pool_registry_mutex);
            g_pools[g_pool_count++] = pool;
        }

        PHX_LOG_INFO(k_log,
            "Created pool '{}' — {} threads, low queue: {}",
            pool->name,
            pool->num_threads,
            pool->has_low_queue ? "YES" : "NO");
            
        return pool;
    }

    void DestroyThreadPool(ThreadPoolHandle pool)
    {
        // Drain the pool first
        Wait(pool);

        pool->wake_condition.notify_all();
        for (u32 t = 0; t < pool->num_threads; t++)
            pool->worker_threads[t].join();

        phx_delete(*g_allocator, pool->worker_threads);
        phx_delete(*g_allocator, pool->high_queues);
        if (pool->low_queues)
            phx_delete(*g_allocator, pool->low_queues);

        // Remove from registry
        {
            std::scoped_lock _(g_pool_registry_mutex);
            for (u32 i = 0; i < g_pool_count; i++)
            {
                if (g_pools[i] == pool)
                {
                    g_pools[i] = g_pools[--g_pool_count];
                    g_pools[g_pool_count] = nullptr;
                    break;
                }
            }
        }

        phx_delete(*g_allocator, pool);
    }

    // ── Submit ────────────────────────────────────────────────────────────────

    void Submit(JobFn fn, Barrier* barrier, Priority priority)
    {
        PHX_ASSERT(g_core_pool != nullptr);
        Submit(std::move(fn), g_core_pool, barrier, priority);
    }

    void Submit(JobFn fn, ThreadPoolHandle pool, Barrier* barrier, Priority priority)
    {
        PHX_ASSERT(pool != nullptr);

        // If no threads, execute inline so caller always gets guaranteed completion
        if (pool->num_threads < 1)
        {
            fn();
            if (barrier) barrier->Signal();
            return;
        }

        Job job;
        job.fn      = std::move(fn);
        job.barrier = barrier;

        // Add to pool-wide barrier BEFORE pushing — prevents a race where the
        // task completes and signals before the counter has been incremented
        pool->pool_barrier.Add();

        if (barrier) 
            barrier->Add();

        pool->queued_jobs.fetch_add(1, std::memory_order_relaxed);

        JobQueue* queues = (priority == Priority::Low && pool->has_low_queue)
            ? pool->low_queues
            : pool->high_queues;

        u32 slot = pool->next_queue.fetch_add(1, std::memory_order_relaxed) % pool->num_threads;

        if (!queues[slot].Push(std::move(job)))
        {
            PHX_LOG_WARN(k_log, "Queue full on pool '{}' — executing inline", pool->name);
            pool->queued_jobs.fetch_sub(1, std::memory_order_relaxed);
            pool->pool_barrier.Signal();
            job.Execute();

            return;
        }

        pool->wake_condition.notify_one();
    }

    void Dispatch(DispatchFn fn, u32 total_count, u32 group_size, Barrier& barrier, Priority priority)
    {
        PHX_ASSERT(g_core_pool != nullptr);
        Dispatch(std::move(fn), total_count, group_size, g_core_pool, barrier, priority);
    }

    void Dispatch(DispatchFn fn, u32 total_count, u32 group_size, ThreadPoolHandle pool, Barrier& barrier, Priority priority)
    {
        PHX_ASSERT(pool != nullptr);
        PHX_ASSERT(total_count > 0);
        PHX_ASSERT(group_size  > 0);

        const u32 group_count = (total_count + group_size - 1) / group_size;

        for (u32 g_idx = 0; g_idx < group_count; g_idx++)
        {
            // Capture everything by value — each group is self-contained
            Submit(
                [fn, g_idx, group_size, group_count, total_count]()
                {
                    const u32 start = g_idx * group_size;
                    const u32 end   = std::min(start + group_size, total_count);

                    for (u32 local = 0; local < (end - start); local++)
                    {
                        DispatchId id = {
                            .global_index = start + local,
                            .group_index  = g_idx,
                            .local_index  = local,
                            .group_count  = group_count,
                            .total_count  = total_count
                        };
                        
                        fn(id);
                    }
                },
                pool,
                &barrier,
                priority);
        }
    }

    // ── Wait ──────────────────────────────────────────────────────────────────

    void Wait()
    {
        std::scoped_lock _(g_pool_registry_mutex);
        for (u32 i = 0; i < g_pool_count; i++)
        {
            if (g_pools[i])
                WaitOnBarrier(g_pools[i], g_pools[i]->pool_barrier);
        }
    }

    void Wait(ThreadPoolHandle pool)
    {
        PHX_ASSERT(pool != nullptr);
        WaitOnBarrier(pool, pool->pool_barrier);
    }

    void Wait(Barrier& barrier)
    {
        PHX_ASSERT(g_core_pool != nullptr);
        WaitOnBarrier(g_core_pool, barrier);
    }

    void Wait(Barrier& barrier, ThreadPoolHandle pool)
    {
        PHX_ASSERT(pool != nullptr);
        WaitOnBarrier(pool, barrier);
    }

    // ── IsBusy ────────────────────────────────────────────────────────────────

    bool IsBusy()
    {
        std::scoped_lock _(g_pool_registry_mutex);
        for (u32 i = 0; i < g_pool_count; i++)
        {
            if (g_pools[i] && g_pools[i]->pool_barrier.IsBusy())
                return true;
        }
        return false;
    }

    bool IsBusy(ThreadPoolHandle pool)
    {
        PHX_ASSERT(pool != nullptr);
        return pool->pool_barrier.IsBusy();
    }

    // ── Flush ─────────────────────────────────────────────────────────────────

    void Flush()
    {
        Wait();
    }

    // ── Query ─────────────────────────────────────────────────────────────────

    u32 GetThreadCount()
    {
        PHX_ASSERT(g_core_pool != nullptr);
        return g_core_pool->num_threads;
    }

    u32 GetThreadCount(ThreadPoolHandle pool)
    {
        PHX_ASSERT(pool != nullptr);
        return pool->num_threads;
    }

    u32 GetTotalThreadCount()
    {
        return g_global_thread_ctr;
    }

    u32 GetNumCores()
    {
        return std::thread::hardware_concurrency();
    }

    // ── Debug ─────────────────────────────────────────────────────────────────

    void DebugPrintPoolStatus(ThreadPoolHandle pool)
    {
        PHX_ASSERT(pool != nullptr);

        PHX_LOG_WARN(k_log, "┌─────────────────────────────────────────┐");
        PHX_LOG_WARN(k_log, "│  Pool: {:32s} │", pool->name);
        PHX_LOG_WARN(k_log, "├───────────────────────┬─────────────────┤");
        PHX_LOG_WARN(k_log, "│  Threads              │ {:<16}│", pool->num_threads);
        PHX_LOG_WARN(k_log, "│  Queued Jobs          │ {:<16}│", pool->queued_jobs.load());
        PHX_LOG_WARN(k_log, "│  Barrier Counter      │ {:<16}│", pool->pool_barrier.counter.load());
        PHX_LOG_WARN(k_log, "│  Has Low Queue        │ {:<16}│", pool->has_low_queue ? "YES" : "NO");
        PHX_LOG_WARN(k_log, "├───────────────────────┼─────────────────┤");

        for (u32 t = 0; t < pool->num_threads; t++)
        {
            PHX_LOG_WARN(k_log, "│  High Queue[{:<2}]        │ {:<16}│",
                t, pool->high_queues[t].Size());

            if (pool->has_low_queue && pool->low_queues)
                PHX_LOG_WARN(k_log, "│  Low  Queue[{:<2}]        │ {:<16}│",
                    t, pool->low_queues[t].Size());
        }

        PHX_LOG_WARN(k_log, "└───────────────────────┴─────────────────┘");
    }
}
