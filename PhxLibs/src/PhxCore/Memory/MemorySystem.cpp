#include <PhxCore/PhxCore_pch.h>

#include "MemorySystem.h"

namespace
{
    MemorySystemDescriptor g_desc;
    phx::MainArena g_mainArenaInstance;
    thread_local phx::ThreadFrameArena g_threadFrameArenaInstance;
    thread_local bool g_threadFrameArenaIsInitialized = false;
    size_t g_deafultFrameArenaSizePerThread = 0;
}

using namespace phx;


void MemorySystem::Initialize(MemorySystemDescriptor const& desc)
{
    g_desc = desc;
    g_mainArenaInstance.Initialize(desc.MainArenaReserveBytes, desc.MainArenaInitialCommitBytes);

    EnsureThreadFrameArenaInitialized();
}

void MemorySystem::Shutdown()
{
    ShutdownCurrentThreadFrameArena();
    g_mainArenaInstance.Shutdown();
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
