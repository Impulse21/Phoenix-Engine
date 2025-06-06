#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/Memory/MemoryUtils.h>
#include "ThreadFrameArena.h"

#define phx_new_frame new (FrameMemoryManager::GetCurrentThreadArena())

namespace phx
{

	struct MemoryStatistics
	{
		size_t AllocatedBytes = 0;
		size_t TotalBytes = 0;
		size_t AllocationCount = 0;

		void Add(size_t a)
		{
			if (a)
			{
				AllocatedBytes += a;
				AllocationCount++;
			}
		}
	};

	struct FrameMemoryDescriptor
	{
		size_t FrameAreaReservedBytesPerThread = 256_MiB;
		size_t FrameAreaInitialCommitPerThread = 4_MiB;
	};

    namespace FrameMemoryManager
    {
		void Initialize(FrameMemoryDescriptor const& desc);
		void Shutdown();

		bool IsInitialized();
		void EnsureThreadFrameArenaInitialized();
		void ShutdownCurrentThreadFrameArena();
		void ResetCurrentThreadFrameAreana();

		ThreadFrameArena& GetCurrentThreadArena();
    }
}

inline void* operator new(size_t size, phx::ThreadFrameArena& allocator)
{
	return allocator.Allocate(size, alignof(std::max_align_t));
}

inline void operator delete(void* ptr, phx::ThreadFrameArena& allocator) noexcept 
{
	allocator.Deallocate(ptr);
}

inline void* operator new[](size_t size, phx::ThreadFrameArena& allocator)
{
	return allocator.Allocate(size, alignof(std::max_align_t));
}

inline void operator delete[](void* ptr, phx::ThreadFrameArena& allocator) noexcept
{
	allocator.Deallocate(ptr); 
}
