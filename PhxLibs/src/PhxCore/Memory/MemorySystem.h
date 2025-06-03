#pragma once

#include "PhxCore/Base.h"
#include <PhxCore/Memory/MemoryArenaInterfaces.h>

#define phx_new new
#define phx_delete delete
#define phx_new_frame new (MemorySystem::GetCurrentThreadArena())

namespace phx
{
	template<typename T, typename U>
	constexpr T AlignUp(T Size, U Alignment)
	{
		return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
	}
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

	struct MemorySystemDescriptor
	{
		size_t MainArenaReserveBytes = 2_GiB;
		size_t MainArenaInitialCommitBytes = 64_MiB;
		size_t FrameAreaReservedBytesPerThread = 16_MiB;
		size_t FrameAreaInitialCommitPerThread = 4_MiB;
	};


	// Internal state declaration (defined in MemoryManager.cpp)
	struct MemorySystemInternalState;
    namespace MemorySystem
    {
		void Initialize(MemorySystemDescriptor const& desc);
		void Shutdown();

		void EnsureThreadFrameArenaInitialized();
		void ShutdownCurrentThreadFrameArena();
		void ResetCurrentThreadFrameAreana();

		MainArena& GetMainArena();
		ThreadFrameArena& GetCurrentThreadArena();


		// --- (Optional) Helper for Placement New with Frame Arena ---
		// struct FrameArenaPlacementTag {};
		// extern FrameArenaPlacementTag frame_arena_tag; // Usage: new (MemoryManager::frame_arena_tag) MyType;
    }
}


[[nodiscard]] void* operator new(size_t size);
[[nodiscard]] void* operator new[](size_t size);

void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;

// C++14 Sized Deletion
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;

// C++17 Aligned Allocation/Deallocation
#if __cplusplus >= 201703L // Check for C++17 or later
[[nodiscard]] void* operator new(size_t size, std::align_val_t al);
[[nodiscard]] void* operator new[](size_t size, std::align_val_t al);

void operator delete(void* ptr, std::align_val_t al) noexcept;
void operator delete[](void* ptr, std::align_val_t al) noexcept;

void operator delete(void* ptr, size_t size, std::align_val_t al) noexcept;
void operator delete[](void* ptr, size_t size, std::align_val_t al) noexcept;
#endif

inline void* operator new(size_t size, phx::ThreadFrameArena& allocator) 
{
	return allocator.Allocate(size, alignof(std::max_align_t));
}

inline void operator delete(void* ptr, phx::ThreadFrameArena& allocator) noexcept 
{
	allocator.Deallocate(ptr); // Size unknown
}

