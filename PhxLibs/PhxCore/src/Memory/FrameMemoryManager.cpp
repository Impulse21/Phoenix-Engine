#include "PhxCore_pch.h"

#include <PhxCore/Memory/FrameMemoryManager.h>
#include <PhxCore/Memory/ThreadFrameArena.h>

using namespace phx;

namespace
{
    FrameMemoryDescriptor g_desc;
    bool g_is_initialized = false;
    thread_local phx::ThreadFrameArena g_threadFrameArenaInstance;
    thread_local bool g_threadFrameArenaIsInitialized = false;
}


void FrameMemoryManager::Initialize(FrameMemoryDescriptor const& desc)
{
    g_desc = desc;

    EnsureThreadFrameArenaInitialized();

    g_is_initialized = true;
}

void FrameMemoryManager::Shutdown()
{
    ShutdownCurrentThreadFrameArena();

    g_is_initialized = false;
}

bool phx::FrameMemoryManager::IsInitialized()
{
    return g_is_initialized;
}

void phx::FrameMemoryManager::EnsureThreadFrameArenaInitialized()
{
    if (!g_threadFrameArenaIsInitialized)
    {
        g_threadFrameArenaInstance.Initialize(
            g_desc.FrameAreaReservedBytesPerThread,
            g_desc.FrameAreaInitialCommitPerThread);

        g_threadFrameArenaIsInitialized = true;
    }
}

void phx::FrameMemoryManager::ShutdownCurrentThreadFrameArena()
{
    g_threadFrameArenaInstance.Shutdown();
}

void phx::FrameMemoryManager::ResetCurrentThreadFrameAreana()
{
    g_threadFrameArenaInstance.Reset();
}

ThreadFrameArena& phx::FrameMemoryManager::GetCurrentThreadArena()
{
    return g_threadFrameArenaInstance;
}

ThreadFrameArena* phx::FrameMemoryManager::GetCurrentThreadArenaPtr()
{
    return &g_threadFrameArenaInstance;
}
