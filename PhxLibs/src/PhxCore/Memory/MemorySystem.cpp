#include <PhxCore/PhxCore_pch.h>

#include "MemorySystem.h"

using namespace phx;

namespace
{
    MemorySystemDescriptor g_desc;
    phx::MainArena g_mainArenaInstance;
    bool g_is_initialized = false;
    thread_local phx::ThreadFrameArena g_threadFrameArenaInstance;
    thread_local bool g_threadFrameArenaIsInitialized = false;
}


void MemorySystem::Initialize(MemorySystemDescriptor const& desc)
{
    g_desc = desc;
    g_mainArenaInstance.Initialize(desc.MainArenaReserveBytes, desc.MainArenaInitialCommitBytes);

    EnsureThreadFrameArenaInitialized();

    g_is_initialized = true;
}

void MemorySystem::Shutdown()
{
    ShutdownCurrentThreadFrameArena();
    g_mainArenaInstance.Shutdown();

    g_is_initialized = false;
}

bool phx::MemorySystem::IsInitialized()
{
    return g_is_initialized;
}

void phx::MemorySystem::EnsureThreadFrameArenaInitialized()
{
    if (!g_threadFrameArenaIsInitialized)
    {
        g_threadFrameArenaInstance.Initialize(
            g_desc.FrameAreaReservedBytesPerThread,
            g_desc.FrameAreaInitialCommitPerThread);

        g_threadFrameArenaIsInitialized = true;
    }
}

void phx::MemorySystem::ShutdownCurrentThreadFrameArena()
{
    g_threadFrameArenaInstance.Shutdown();
}

void phx::MemorySystem::ResetCurrentThreadFrameAreana()
{
    g_threadFrameArenaInstance.Reset();
}

MainArena& phx::MemorySystem::GetMainArena()
{
    return g_mainArenaInstance;
}

ThreadFrameArena& phx::MemorySystem::GetCurrentThreadArena()
{
    return g_threadFrameArenaInstance;
}
