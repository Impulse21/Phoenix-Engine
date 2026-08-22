#pragma once

#include <PhxEngine/Core/FixedCallable.h>
#include <PhxEngine/Memory/IHeapAllocator.h>

#include <PhxEngine/Platform/Thread.h>

#include <atomic>

namespace phx
{
    using TaskFn     = FixedCallable<64>;
    using DispatchFn = FixedCallable<64>;

    struct ThreadPool;
    using ThreadPoolHandle = ThreadPool*;
    

    struct ThreadPoolDescriptor
    {
        const char*                 name            = "";
        u32                         thread_count    = 0; // 0 = hardware_concurrency - 1
        platform::Thread::Priority  priority        = platform::Thread::Priority::Normal;
        bool                        has_low_queue   = false;
    };
    
    struct Barrier
    {
        std::atomic_int counter = 0;

        void Add    ()       { counter.fetch_add(1, std::memory_order_relaxed); }
        void Signal ()       { counter.fetch_sub(1, std::memory_order_release); }
        bool IsBusy () const { return counter.load(std::memory_order_acquire) > 0; }

        PHX_NO_COPY_NO_MOVE(Barrier);

        Barrier() = default;
    };

    struct DispatchId
    {
        u32 global_index;   // index across all groups
        u32 group_index;    // which group this element belongs to
        u32 local_index;    // index within the group
        u32 group_count;    // total groups submitted
        u32 total_count;    // total elements submitted
    };

    namespace Jobs
    {
        enum class Priority : u8 
        { 
            High,
            Low 
        };

        void Initialize(IHeapAllocator* allocator);
        void Shutdown();

        void BeginFrame();

        ThreadPoolHandle CreateThreadPool(const ThreadPoolDescriptor& desc);
        void DestroyThreadPool(ThreadPoolHandle pool);

        void Submit(
            TaskFn           fn,
            Barrier*         barrier  = nullptr,
            Priority         priority = Priority::High);

        void Submit(
            TaskFn           fn,
            ThreadPoolHandle pool,
            Barrier*         barrier  = nullptr,
            Priority         priority = Priority::High);

        void Dispatch(
            DispatchFn       fn,
            u32              total_count,
            u32              group_size,
            Barrier&         barrier,
            Priority         priority = Priority::High);

        void Dispatch(
            DispatchFn       fn,
            u32              total_count,
            u32              group_size,
            ThreadPoolHandle pool,
            Barrier&         barrier,
            Priority         priority = Priority::High);

        void Wait();
        void Wait(ThreadPoolHandle pool);

        void Wait(Barrier& barrier);
        void Wait(Barrier& barrier, ThreadPoolHandle pool);

        bool IsBusy();
        bool IsBusy(ThreadPoolHandle pool);

        void Flush();

        u32  GetThreadCount     ();
        u32  GetThreadCount     (ThreadPoolHandle pool);
        u32  GetTotalThreadCount();
        u32  GetNumCores        ();

        void DebugPrintPoolStatus(ThreadPoolHandle pool);
    }
}