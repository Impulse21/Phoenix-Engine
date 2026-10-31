// Author: Claude Code (Free)
// JobsSmokeTests.cpp
//
// Framework-free smoke tests for phx::Jobs. Returns 0 on success, 1 if any
// CHECK failed. A deadlock or lost wakeup aborts the process via the watchdog
// instead of freezing the run.
//
// Usage:
//   PhxTest_JobSystem              10 rounds on each of 1, 2, 4 and default workers
//   PhxTest_JobSystem 100          100 rounds each (soak run)
//   PhxTest_JobSystem 100 1        100 rounds on exactly 1 worker
//
// Few workers is where the wait/queue bugs show up: with 23 workers there is
// always an idle one to rescue a stuck wait.
//
// Assumes the interface:
//   Jobs::Initialize(u32), Jobs::Shutdown()
//   Jobs::SubmitDetached(Fn, Priority = High)   -- no barrier, pool-wide Wait() only
//   Jobs::Submit(Fn, Barrier&, Priority = High)
//   Jobs::Dispatch(const TFn&, total, group_size, Barrier&, Priority = High)
//   Jobs::Wait(const Barrier&)   -- runs jobs while waiting, any thread
//   Jobs::Wait()                 -- pool-wide, main thread only
//   Jobs::GetWorkerCount(), Jobs::GetCurrentThreadSlot()
//
// Job lambdas must stay <= 64 bytes and trivially copyable: capture
// references/pointers/ints only.

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Jobs.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <thread>
#include <vector>

using namespace phx;
using namespace std::chrono_literals;

namespace
{
    std::atomic<int> g_failures = 0;
    bool g_quiet = false;   // only the first round prints RUN/OK lines
    int  g_round = 0;

#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond))                                                                 \
        {                                                                            \
            std::printf("    CHECK FAILED %s:%d  %s\n", __FILE__, __LINE__, #cond);  \
            g_failures.fetch_add(1);                                                 \
        }                                                                            \
    } while (0)

    // Runs `body` on the calling thread. A watchdog thread aborts the process
    // if it doesn't finish in time (deadlock, lost wakeup, dropped job).
    template<typename TFn>
    void Run(const char* name, std::chrono::seconds timeout, TFn&& body)
    {
        if (!g_quiet)
        {
            std::printf("[ RUN  ] %s\n", name);
            std::fflush(stdout);
        }

        std::atomic<bool> finished = false;
        std::thread watchdog([&]
        {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            while (!finished.load())
            {
                if (std::chrono::steady_clock::now() > deadline)
                {
                    std::printf("[ HANG ] %s (round %d, > %llds) -- deadlock, lost wakeup or dropped job\n",
                        name, g_round, static_cast<long long>(timeout.count()));
                    std::fflush(stdout);
                    std::abort();
                }
                std::this_thread::sleep_for(10ms);
            }
        });

        const int failures_before = g_failures.load();
        body();
        finished.store(true);
        watchdog.join();

        if (g_failures.load() != failures_before)
            std::printf("[ FAIL ] %s (round %d)\n", name, g_round);
        else if (!g_quiet)
            std::printf("[  OK  ] %s\n", name);
    }

    // Busy-wait: gives workers time to fall asleep without sleep_for's
    // coarse timer granularity on Windows.
    void SpinFor(std::chrono::microseconds duration)
    {
        const auto until = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < until) {}
    }

    // ------------------------------------------------------------------

    // Every job runs exactly once; Wait(barrier) returns only after all are done.
    void TestSubmitWithBarrier()
    {
        constexpr int kJobs = 10000;
        std::atomic<int> count = 0;
        Jobs::Barrier done;

        for (int i = 0; i < kJobs; ++i)
            Jobs::Submit([&count] { count.fetch_add(1); }, done);

        Jobs::Wait(done);
        CHECK(count.load() == kJobs);
        CHECK(!done.IsNotCleared());
    }

    // SubmitDetached + pool-wide Wait() from the main thread.
    void TestPoolWideWait()
    {
        constexpr int kJobs = 5000;
        std::atomic<int> count = 0;

        for (int i = 0; i < kJobs; ++i)
            Jobs::SubmitDetached([&count] { count.fetch_add(1); });

        Jobs::Wait();
        CHECK(count.load() == kJobs);
    }

    void TestLowPriority()
    {
        constexpr int kJobs = 2000;
        std::atomic<int> count = 0;
        Jobs::Barrier done;

        for (int i = 0; i < kJobs; ++i)
            Jobs::Submit([&count] { count.fetch_add(1); }, done, Jobs::Priority::Low);

        Jobs::Wait(done);
        CHECK(count.load() == kJobs);
    }

    // Each index visited exactly once, DispatchId fields self-consistent,
    // across group sizes that do and don't divide the total.
    void TestDispatchCoverage()
    {
        constexpr u32 kTotal = 1000;

        for (const u32 group_size : { 1u, 7u, 64u, 999u, 1000u, 5000u })
        {
            const u32 expected_groups = (kTotal + group_size - 1) / group_size;

            std::vector<std::atomic<int>> hits(kTotal);
            for (auto& h : hits) h.store(0);

            std::atomic<int> bad_ids = 0;
            Jobs::Barrier done;

            auto fn = [&hits, &bad_ids, group_size, expected_groups](const Jobs::DispatchId& id)
            {
                if (id.total_count != kTotal ||
                    id.group_count != expected_groups ||
                    id.group_index * group_size + id.local_index != id.global_index ||
                    id.global_index >= kTotal)
                {
                    bad_ids.fetch_add(1);
                    return;
                }
                hits[id.global_index].fetch_add(1);
            };

            Jobs::Dispatch(fn, kTotal, group_size, done);
            Jobs::Wait(done);   // fn must outlive this

            int missed_or_doubled = 0;
            for (u32 i = 0; i < kTotal; ++i)
                if (hits[i].load() != 1) ++missed_or_doubled;

            CHECK(bad_ids.load() == 0);
            CHECK(missed_or_doubled == 0);
        }
    }

    // Jobs spawning jobs on the same barrier: children Add() before the parent
    // Signal()s, so the count can't reach zero early.
    void TestNestedSubmit()
    {
        constexpr int kParents = 200;
        constexpr int kChildren = 50;
        std::atomic<int> leaves = 0;
        Jobs::Barrier done;

        for (int p = 0; p < kParents; ++p)
        {
            Jobs::Submit([&leaves, &done]
            {
                for (int c = 0; c < kChildren; ++c)
                    Jobs::Submit([&leaves] { leaves.fetch_add(1); }, done);
            }, done);
        }

        Jobs::Wait(done);
        CHECK(leaves.load() == kParents * kChildren);
    }

    // The deadlock case: more waiting jobs than workers, each waiting on its own
    // children. Only passes if Wait() runs other jobs while waiting.
    void TestWaitInsideJob()
    {
        const int parents = static_cast<int>(Jobs::GetWorkerCount()) * 4;
        constexpr int kChildren = 100;
        std::atomic<int> leaves = 0;
        Jobs::Barrier all;

        for (int p = 0; p < parents; ++p)
        {
            Jobs::Submit([&leaves]
            {
                Jobs::Barrier mine;   // on this job's stack; Wait keeps it alive
                for (int c = 0; c < kChildren; ++c)
                    Jobs::Submit([&leaves] { leaves.fetch_add(1); }, mine);
                Jobs::Wait(mine);
            }, all);
        }

        Jobs::Wait(all);
        CHECK(leaves.load() == parents * kChildren);
    }

    // Block every worker, flood far past the ring capacity, release. Nothing
    // may be dropped (a dropped job leaves the barrier above zero -> hang).
    void TestQueueOverflowDropsNothing()
    {
        const u32 workers = Jobs::GetWorkerCount();
        const u32 flood_jobs = workers * 512 + 1000;   // > 256 per queue

        std::atomic<u32> blocked = 0;
        std::atomic<bool> release = false;
        std::atomic<u32> count = 0;
        Jobs::Barrier blockers, flood;

        for (u32 w = 0; w < workers; ++w)
        {
            Jobs::Submit([&blocked, &release]
            {
                blocked.fetch_add(1);
                while (!release.load()) std::this_thread::yield();
            }, blockers);
        }
        while (blocked.load() < workers) std::this_thread::yield();   // all workers parked

        for (u32 i = 0; i < flood_jobs; ++i)
            Jobs::Submit([&count] { count.fetch_add(1); }, flood);

        release.store(true);
        Jobs::Wait(flood);
        Jobs::Wait(blockers);
        CHECK(count.load() == flood_jobs);
    }

    // Idle workers must wake on their own: main spins on a flag and never calls
    // Wait(), so it can't run the job itself. A lost wakeup shows up as a timeout.
    // Probabilistic -- the repeat rounds are what make it meaningful.
    void TestIdleWorkersWake()
    {
        std::mt19937 rng(1234 + g_round);

        for (int i = 0; i < 1000; ++i)
        {
            std::atomic<bool> ran = false;
            Jobs::SubmitDetached([&ran] { ran.store(true); });

            const auto deadline = std::chrono::steady_clock::now() + 5s;
            while (!ran.load())
            {
                if (std::chrono::steady_clock::now() > deadline)
                {
                    std::printf("    job %d never ran (lost wakeup?)\n", i);
                    CHECK(false);
                    return;
                }
                std::this_thread::yield();
            }
            SpinFor(std::chrono::microseconds(rng() % 300));   // let workers fall asleep
        }
        Jobs::Wait();
    }

    // Slot is in [0, workers] everywhere; the (non-worker) main thread gets exactly `workers`.
    // Main can legitimately run jobs inside Wait(), so jobs may also see `workers`.
    void TestThreadSlots()
    {
        const u32 workers = Jobs::GetWorkerCount();
        CHECK(workers > 0);
        CHECK(Jobs::GetCurrentThreadSlot() == workers);

        std::atomic<int> bad = 0;
        Jobs::Barrier done;
        for (int i = 0; i < 2000; ++i)
            Jobs::Submit([&bad, workers]
            {
                if (Jobs::GetCurrentThreadSlot() > workers) bad.fetch_add(1);
            }, done);

        Jobs::Wait(done);
        CHECK(bad.load() == 0);
    }

    // The real usage pattern, many frames in a row:
    //   PreRender: main Dispatches per-view work and waits for it
    //   Update:    a job on the pool that spawns children and runs a nested
    //              Dispatch, waiting on its own batch before returning
    //   Render:    main does its own work at the same time
    //   End:       main waits on the frame barrier
    void TestFrameLoopPattern()
    {
        constexpr int kFrames   = 200;
        constexpr int kViews    = 8;
        constexpr int kChildren = 32;
        constexpr int kNested   = 100;

        for (int frame = 0; frame < kFrames; ++frame)
        {
            const int failures_before = g_failures.load();

            std::atomic<int> culled = 0, children = 0, nested = 0;

            Jobs::Barrier prerender_done;
            auto cull_view = [&culled](const Jobs::DispatchId&) { culled.fetch_add(1); };
            Jobs::Dispatch(cull_view, kViews, 1, prerender_done);
            Jobs::Wait(prerender_done);
            CHECK(culled.load() == kViews);

            Jobs::Barrier frame_done;
            Jobs::Submit([&frame_done, &children, &nested]
            {
                for (int c = 0; c < kChildren; ++c)
                    Jobs::Submit([&children] { children.fetch_add(1); }, frame_done);

                // `work` is local to this job, so this job must Wait for its own
                // batch before returning or the callable would dangle.
                Jobs::Barrier mine;
                auto work = [&nested](const Jobs::DispatchId&) { nested.fetch_add(1); };
                Jobs::Dispatch(work, kNested, 10, mine);
                Jobs::Wait(mine);
            }, frame_done);

            SpinFor(20us);   // "Render" on main

            Jobs::Wait(frame_done);
            CHECK(children.load() == kChildren);
            CHECK(nested.load() == kNested);

            if (g_failures.load() != failures_before)
                return;   // don't print 200 identical failures
        }

        Jobs::Wait();
    }

    // Shutdown with idle (sleeping) workers must not hang, and re-init must work.
    // Runs on its own, outside the per-worker-count sweep. Leaves the system initialized.
    void TestInitShutdownCycles()
    {
        std::mt19937 rng(42);
        Jobs::Shutdown();

        for (int i = 0; i < 50; ++i)
        {
            Jobs::Initialize(2 + (i % 3));

            std::atomic<int> count = 0;
            Jobs::Barrier done;
            for (int j = 0; j < 100; ++j)
                Jobs::Submit([&count] { count.fetch_add(1); }, done);
            Jobs::Wait(done);
            CHECK(count.load() == 100);

            SpinFor(std::chrono::microseconds(rng() % 2000));   // let workers go to sleep
            Jobs::Shutdown();
        }

        Jobs::Initialize();
    }

    void RunSuite()
    {
        Run("SubmitWithBarrier",         10s, TestSubmitWithBarrier);
        Run("PoolWideWait",              10s, TestPoolWideWait);
        Run("LowPriority",               10s, TestLowPriority);
        Run("DispatchCoverage",          10s, TestDispatchCoverage);
        Run("NestedSubmit",              10s, TestNestedSubmit);
        Run("WaitInsideJob",             10s, TestWaitInsideJob);
        Run("QueueOverflowDropsNothing", 10s, TestQueueOverflowDropsNothing);
        Run("IdleWorkersWake",           30s, TestIdleWorkersWake);
        Run("ThreadSlots",               10s, TestThreadSlots);
        Run("FrameLoopPattern",          20s, TestFrameLoopPattern);
    }
}

int main(int argc, char** argv)
{
    const int rounds = argc > 1 ? std::atoi(argv[1]) : 10;

    std::vector<u32> worker_counts = { 1, 2, 4, 0 };   // 0 = Initialize's default
    if (argc > 2)
        worker_counts = { static_cast<u32>(std::atoi(argv[2])) };

    // If Jobs::Initialize depends on engine bring-up (CVars, ...), do that first.

    for (const u32 requested : worker_counts)
    {
        Jobs::Initialize(requested);
        std::printf("=== %u workers, %d rounds ===\n", Jobs::GetWorkerCount(), rounds);

        for (g_round = 0; g_round < rounds && g_failures.load() == 0; ++g_round)
        {
            g_quiet = (g_round > 0);
            RunSuite();
        }

        Jobs::Shutdown();
        if (g_failures.load() != 0)
            break;
    }

    if (g_failures.load() == 0)
    {
        g_quiet = false;
        g_round = 0;
        Run("InitShutdownCycles", 30s, TestInitShutdownCycles);
        Jobs::Shutdown();
    }

    const int failures = g_failures.load();
    std::printf("\n%s (%d failed checks)\n", failures == 0 ? "ALL PASSED" : "FAILURES", failures);
    return failures == 0 ? 0 : 1;
}