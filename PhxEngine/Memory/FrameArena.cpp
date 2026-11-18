#include "FrameArena.h"

using namespace phx;

void phx::FrameArena::Initialize(VirtualPageSource& source)
{
    PHX_ASSERT(m_source == nullptr && "FrameArena initialized twice");
    PHX_ASSERT(source.IsInitialized());

    m_source = &source;
    for (MemoryChunk*& list : m_live)
        list = nullptr;
}

void phx::FrameArena::Shutdown()
{
    std::scoped_lock lock(m_mutex);
    if (m_source == nullptr)
        return;

    for (MemoryChunk*& list : m_live)
    {
        m_source->ReleaseChunkList(list);
        list = nullptr;
    }
    m_source = nullptr;
}

FrameArena::FrameChunk phx::FrameArena::AcquireChunk(usize min_usable_bytes)
{
    // Lock order is always arena -> source; the source never calls back in.
    std::scoped_lock lock(m_mutex);
    PHX_ASSERT(m_source != nullptr && "FrameArena used before Initialize / after Shutdown");

    // Read the generation under the lock so the chunk is filed under the
    // same generation the caller tags it with, even if EndFrame races us.
    const u64 generation = m_generation.load(std::memory_order_relaxed);

    MemoryChunk* chunk = m_source->AcquireChunk(min_usable_bytes);
    MemoryChunk*& list = m_live[generation % kFrameLatency];
    chunk->next = list;
    list = chunk;

    return { .chunk = chunk, .generation = generation };
}

void phx::FrameArena::EndFrame()
{
    std::scoped_lock lock(m_mutex);
    PHX_ASSERT(m_source != nullptr);

    // Ending generation G: release generation G + 1 - kFrameLatency, whose
    // slot generation G + 1 is about to reuse. With kFrameLatency == 1
    // that's G itself.
    const u64 generation = m_generation.load(std::memory_order_relaxed);
    MemoryChunk*& oldest = m_live[(generation + 1) % kFrameLatency];
    m_source->ReleaseChunkList(oldest);
    oldest = nullptr;

    m_generation.store(generation + 1, std::memory_order_release);
}
