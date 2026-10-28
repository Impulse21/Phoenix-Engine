#include "Jobs.h"

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/CVar.h>
#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Core/RingBuffer.h>

#if defined(PHX_PROFILING_ENABLED)
    #include <tracy/Tracy.hpp>
#endif

#include <memory>
#include <mutex>
#include <thread>
#include <algorithm>
#include <condition_variable>

PHX_CVAR_INT(jobs_thread_count, 0, "Jobs worker thread count (0 = hardware_concurrency)");

using namespace phx;

// -- Internal Types ---
namespace
{
    struct BarrierSignaller
	{
    	Jobs::Barrier* barrier;
    	~BarrierSignaller() 
        { 
            if (barrier)
                barrier->Signal(); 
        }
	};

	struct Job
	{
		Jobs::Fn fn;
        Jobs::Barrier* global_barrier = nullptr;
        Jobs::Barrier* user_barrier   = nullptr;

		void Execute()
		{
			BarrierSignaller signaller{ .barrier = global_barrier };
			BarrierSignaller signaller_user{ .barrier = user_barrier };
			fn();
		}
	};

    using JobQueue = RingBuffer<Job, 256>;
}

// -- Members ----
namespace
{
    constexpr u32 k_not_a_worker = std::numeric_limits<u32>::max();
    bool m_is_alive = false;
    u32 m_num_threads = 0;

    std::unique_ptr<std::thread[]> m_threads;
    EnumArray<std::unique_ptr<JobQueue[]>, Jobs::Priority> m_per_thread_queues;

    // -- Hot Data ---
	std::atomic_uint32_t m_queued_jobs = 0;
	std::atomic_uint32_t m_next_queue = 0;
	std::condition_variable m_wake_condidition;
    std::mutex m_wake_mutex;

	Jobs::Barrier m_global_barrier = {};

    // -- Per thread data ---
    struct ThreadInfo
    {
        u32 worker_thread_id = std::numeric_limits<uint32_t>::max();
    };

	thread_local ThreadInfo m_thread_info;
}

// -- Internal functions ---
static void SubmitImpl(Jobs::Fn fn, Jobs::Barrier* user_barrier, Jobs::Priority priority);
static bool TryRunOneJob(u32 start);

void phx::Jobs::Initialize(u32 thread_count)
{
    m_is_alive = true;
    
    // if zero, set thread count to number of cores
    if (thread_count == 0)
        thread_count = std::thread::hardware_concurrency();

    m_num_threads = std::max(1u, thread_count);

    // -- Construct work queues ---
    m_per_thread_queues[Priority::High] = std::make_unique<JobQueue[]>(m_num_threads);
    m_per_thread_queues[Priority::Low] = std::make_unique<JobQueue[]>(m_num_threads);


    // -- Create threads ---
    m_threads = std::make_unique<std::thread[]>(m_num_threads);

    for (u32 i = 0; i < m_num_threads; ++i)
    {
        // -- Thread core loop ---
        m_threads[i] = std::thread(
            [i]()
            {
                m_thread_info = {
                    .worker_thread_id = i,
                };
                while (m_is_alive)
                {
                    while (TryRunOneJob(i))
                    {
                        // no-op
                    }

                    std::unique_lock<std::mutex> lock(m_wake_mutex);
                    m_wake_condidition.wait(lock, [] { return !m_is_alive || m_queued_jobs.load() > 0; });
                }
            });
    }

}

void phx::Jobs::Shutdown()
{
    if (!m_is_alive) 
        return;

    Wait();
    m_is_alive = false;

    { 
        std::lock_guard<std::mutex> lock(m_wake_mutex); 
    }

    m_wake_condidition.notify_all();

    for (u32 i = 0; i < m_num_threads; ++i)
        m_threads[i].join();

    m_threads.reset();
    m_per_thread_queues[Priority::High].reset();
    m_per_thread_queues[Priority::Low].reset();
    m_num_threads = 0;
}

void phx::Jobs::Submit(Fn fn, Priority priority)
{
    SubmitImpl(fn, nullptr, priority);
}

void phx::Jobs::Submit(Fn fn, Barrier& barrier, Priority priority)
{
    SubmitImpl(fn, &barrier, priority);
}

void phx::Jobs::Wait()
{
    PHX_ASSERT(m_thread_info.worker_thread_id == k_not_a_worker
        && "Jobs::Wait(): pool-wide wait from inside a job waits on itself");
    Wait(m_global_barrier);
}

void phx::Jobs::Wait(const Barrier& barrier)
{
    const u32 start = GetCurrentThreadId();
    while (barrier.IsNotCleared())
    {
        if (!TryRunOneJob(start))
            std::this_thread::yield();
    }
}

u32 phx::Jobs::GetWorkerCount()
{
    return m_num_threads;
}

u32 phx::Jobs::GetCurrentThreadId()
{
    return m_thread_info.worker_thread_id;
}

void SubmitImpl(Jobs::Fn fn, Jobs::Barrier* user_barrier, Jobs::Priority priority)
{
    // If we have no threads, just execute it right away.
    if (m_num_threads < 1) 
    { 
        fn(); return; 
    }

    if (user_barrier)
        user_barrier->Add();
        
    m_global_barrier.Add();
    Job job = { 
            .fn = std::move(fn),
            .global_barrier = &m_global_barrier,
            .user_barrier = user_barrier
    };

    m_queued_jobs.fetch_add(1);

    JobQueue* queues = m_per_thread_queues[priority].get();
    const u32 start = m_next_queue.fetch_add(1);

    bool pushed = false;
    for (u32 i = 0; i < m_num_threads && !pushed; ++i)
        pushed = queues[(start + i) % m_num_threads].Push(job);

    // -- If ring buffer is full, run inline ---
    // TODO: Try next queue
    if (!pushed)
    {
        m_queued_jobs.fetch_sub(1);
        job.Execute();
        return;
    }

    { 
        std::lock_guard<std::mutex> lock(m_wake_mutex);
    }

    m_wake_condidition.notify_one();
}

// -- Private Impl ---
bool TryRunOneJob(u32 start)
{
    const Jobs::Priority order[] = { Jobs::Priority::High, Jobs::Priority::Low };
    for (const Jobs::Priority priority : order)
    {
        JobQueue* queues = m_per_thread_queues[priority].get();
        for (u32 i = 0; i < m_num_threads; ++i)
        {
            Job job;
            if (queues[(start + i) % m_num_threads].Pop(job))
            {
                m_queued_jobs.fetch_sub(1);
                job.Execute();
                return true;
            }
        }
    }

    return false;
}