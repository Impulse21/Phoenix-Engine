#include <PhxCore/PhxCore_pch.h>
#include "MemorySystem.h"
#include "BootstrapAllocator.h"
#include <PhxCore/Base.h>

using namespace phx;


// Helper for delete dispatch
static void DispatchDeallocate(void* ptr) 
{
    if (!ptr)
        return;

    if (!MemorySystem::IsInitialized())
    {
        BootstrapAllocator::Deallocate(ptr);
        return;
    }

    MemorySystem::EnsureThreadFrameArenaInitialized(); // Make sure TLS arena is ready for check
    ThreadFrameArena& frameArena = MemorySystem::GetCurrentThreadArena();
    if (frameArena.IsAddressInRange(ptr))
    {
        frameArena.Deallocate(ptr); // Pass through known info
        return;
    }

    MainArena& main_arena = MemorySystem::GetMainArena();
    if (main_arena.IsAddressInRange(ptr))
    {
        main_arena.Deallocate(ptr); // Pass through known info
        return;
    }

    PHX_CORE_ERROR("MemoryManager Error: Deleting pointer {0} not managed by custom arenas.", ptr);
}


// --- Global new/delete Definitions ---
[[nodiscard]] void* operator new(size_t size) 
{
    // All default 'new' calls go to the MainArena
    return phx::MemorySystem::IsInitialized()
        ? phx::MemorySystem::GetMainArena().Allocate(size, alignof(std::max_align_t))
        : phx::BootstrapAllocator::Allocate(size, alignof(std::max_align_t));
}

[[nodiscard]] void* operator new[](size_t size) 
{
    return phx::MemorySystem::IsInitialized()
        ? phx::MemorySystem::GetMainArena().Allocate(size, alignof(std::max_align_t))
        : phx::BootstrapAllocator::Allocate(size, alignof(std::max_align_t));
}

// C++17 aligned new
#if __cplusplus >= 201703L
[[nodiscard]] void* operator new(size_t size, std::align_val_t al) 
{
    return phx::MemorySystem::IsInitialized()
        ? phx::MemorySystem::GetMainArena().Allocate(size, static_cast<size_t>(al))
        : phx::BootstrapAllocator::Allocate(size, static_cast<size_t>(al));
}
[[nodiscard]] void* operator new[](size_t size, std::align_val_t al) 
{
    return phx::MemorySystem::IsInitialized()
        ? phx::MemorySystem::GetMainArena().Allocate(size, static_cast<size_t>(al))
        : phx::BootstrapAllocator::Allocate(size, static_cast<size_t>(al));
}
#endif


// --- Global delete Definitions ---
void operator delete(void* ptr) noexcept 
{
    DispatchDeallocate(ptr); // Size and specific alignment unknown
}
void operator delete[](void* ptr) noexcept 
{
    DispatchDeallocate(ptr);
}

// C++14 Sized Deletion
void operator delete(void* ptr, size_t) noexcept 
{
    DispatchDeallocate(ptr);
}

void operator delete[](void* ptr, size_t) noexcept 
{
    DispatchDeallocate(ptr);
}


// C++17 Aligned Deletion
#if __cplusplus >= 201703L
void operator delete(void* ptr, std::align_val_t /*al*/) noexcept
{
    DispatchDeallocate(ptr);
}

void operator delete[](void* ptr, std::align_val_t /*al*/) noexcept
{
    DispatchDeallocate(ptr);
}

void operator delete(void* ptr, size_t /*size*/, std::align_val_t /*al*/) noexcept
{
    DispatchDeallocate(ptr);
}

void operator delete[](void* ptr, size_t /*size*/, std::align_val_t /*al*/) noexcept
{
    DispatchDeallocate(ptr);
}

#endif