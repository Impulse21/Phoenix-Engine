#pragma once

#include <functional>
#include <memory>
#include <PhxEngine/Core/FixedCallable.h>

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

    void Submit(Fn fn, Priority priority = Priority::High);
    void Submit(Fn fn, Barrier& barrier, Priority priority = Priority::High);

    template<typename TFn>   // fn is referenced, not copied: it must outlive Wait(barrier)
    void Dispatch(const TFn& fn, u32 total_count, u32 group_size, Barrier& barrier, Priority priority = Priority::High);

    void Wait();
    void Wait(const Barrier& barrier);
    void Signal(Barrier barrier);

    
    u32 GetWorkerCount();
    u32 GetCurrentThreadSlot();
}
