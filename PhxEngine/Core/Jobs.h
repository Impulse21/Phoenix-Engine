#pragma once

#include <PhxEngine/Core/FixedCallable.h>

#include <memory>
#include <algorithm>
#include <atomic>

namespace phx::Jobs
{
    using Fn = FixedCallable<64>;

    enum class Priority
	{
	    High,
		Low,
		Count,
    };

    struct Barrier
    {
		std::atomic_int Counter = 0;

		void Signal() { Counter.fetch_sub(1); }
		void Add() { Counter.fetch_add(1); }
	    bool IsNotCleared() const { return Counter.load() > 0; }
    };

    struct DispatchId
    {
		u32 global_index;	// index across all groups;
		u32 group_index; 	// The group index for this dispatch.
		u32 local_index; 	// The local index within the group.
		u32 group_count; 	// The total number of groups.
		u32 total_count; 	// the Total element count.
    };

    void Initialize(u32 thread_count = 0);
    void Shutdown();

    void SubmitDetached(Fn fn, Priority priority = Priority::High);
    void Submit(Fn fn, Barrier& barrier, Priority priority = Priority::High);

    template<typename TFn>
    void Dispatch(const TFn& fn, u32 total_count, u32 group_size, Barrier& barrier, Priority priority = Priority::High)
    {
        PHX_ASSERT(group_size > 0);
        const u32 group_count = (total_count + group_size - 1) / group_size;

        for (u32 g = 0; g < group_count; ++g)
        {
            Submit([&fn, g, group_size, group_count, total_count] // 8 + 16 = 24 bytes
            {
                const u32 start = g * group_size;
                const u32 end   = std::min(start + group_size, total_count);
                for (u32 i = start; i < end; ++i)
                    fn(DispatchId{ i, g, i - start, group_count, total_count });
            }, barrier, priority);
        }
    }
    // -- Rejects temp fn as it's not kept around ---
    template<typename TFn> void Dispatch(const TFn&&, u32, u32, Barrier&, Priority = Priority::High) = delete;

    void Wait();
    void Wait(const Barrier& barrier);

    
    u32 GetWorkerCount();
    u32 GetCurrentThreadSlot();
}