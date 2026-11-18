#include "VirtualPageSource.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Platform/VirtualMemory.h>

#include <cstdlib>

using namespace phx;

bool phx::VirtualPageSource::Initialize(const Descriptor& desc)
{
    PHX_ASSERT(m_base == nullptr && "VirtualPageSource initialized twice");

    // Chunks are carved back to back from the base, so a chunk size that's a
    // multiple of the allocation granularity keeps every chunk base aligned
    // for commit (page) and for kMaxAlignment.
    const usize granularity = platform::VirtualMemory::GetAllocationGranularity();
    m_chunk_size = AlignUp(desc.chunk_size, granularity);
    m_reserved   = AlignUp(desc.reserve_size, m_chunk_size);

    m_base = static_cast<u8*>(platform::VirtualMemory::Reserve(m_reserved));
    if (m_base == nullptr)
    {
        PHX_LOG_ERROR(Log::Channels::Memory, "Failed to reserve {0} bytes of virtual memory", m_reserved);
        m_reserved = 0;
        return false;
    }

    m_carved      = 0;
    m_committed   = 0;
    m_free_chunks = nullptr;
    m_free_spans  = nullptr;
    return true;
}

void phx::VirtualPageSource::Shutdown()
{
    std::scoped_lock lock(m_mutex);
    if (m_base == nullptr)
        return;

    platform::VirtualMemory::Release(m_base, m_reserved);

    m_base        = nullptr;
    m_reserved    = 0;
    m_carved      = 0;
    m_committed   = 0;
    m_free_chunks = nullptr;
    m_free_spans  = nullptr;
}

MemoryChunk* phx::VirtualPageSource::AcquireChunk(usize min_usable_bytes)
{
    const usize needed = AlignUp(min_usable_bytes + kChunkHeaderSize, m_chunk_size);

    u8* fresh = nullptr;
    {
        std::scoped_lock lock(m_mutex);
        PHX_ASSERT(m_base != nullptr && "VirtualPageSource used before Initialize / after Shutdown");

        // Recycle first.
        if (needed == m_chunk_size && m_free_chunks != nullptr)
        {
            MemoryChunk* chunk = m_free_chunks;
            m_free_chunks = chunk->next;
            chunk->next = nullptr;
            return chunk;
        }

        if (needed > m_chunk_size)
        {
            for (MemoryChunk** link = &m_free_spans; *link != nullptr; link = &(*link)->next)
            {
                MemoryChunk* span = *link;
                if (span->size >= needed)
                {
                    *link = span->next;
                    span->next = nullptr;
                    return span;
                }
            }
        }

        // Carve new pages. The commit happens outside the lock: the range is
        // already ours once m_carved moves past it.
        if (needed > m_reserved - m_carved)
        {
            PHX_LOG_ERROR(Log::Channels::Memory,
                "VirtualPageSource exhausted: requested {0} bytes, {1}/{2} bytes carved",
                needed, m_carved, m_reserved);
            PHX_ASSERT(false);
            std::abort();
        }

        fresh = m_base + m_carved;
        m_carved    += needed;
        m_committed += needed;
    }

    if (!platform::VirtualMemory::Commit(fresh, needed))
    {
        PHX_LOG_ERROR(Log::Channels::Memory, "VirtualPageSource failed to commit {0} bytes", needed);
        PHX_ASSERT(false);
        std::abort();
    }

    MemoryChunk* chunk = ::new (static_cast<void*>(fresh)) MemoryChunk{};
    chunk->size = needed;
    return chunk;
}

void phx::VirtualPageSource::ReleaseChunk(MemoryChunk* chunk)
{
    if (chunk == nullptr)
        return;

    std::scoped_lock lock(m_mutex);

    // After Shutdown the pages are unmapped: don't touch the chunk at all.
    // Late releases (e.g. thread-local allocators destroyed at exit) are dropped.
    if (m_base == nullptr)
        return;

    ReleaseLocked(chunk);
}

void phx::VirtualPageSource::ReleaseChunkList(MemoryChunk* head)
{
    std::scoped_lock lock(m_mutex);
    if (m_base == nullptr)
        return;   // see ReleaseChunk

    while (head != nullptr)
    {
        MemoryChunk* next = head->next;
        ReleaseLocked(head);
        head = next;
    }
}

void phx::VirtualPageSource::ReleaseLocked(MemoryChunk* chunk)
{
    PHX_ASSERT(reinterpret_cast<u8*>(chunk) >= m_base && reinterpret_cast<u8*>(chunk) < m_base + m_carved
        && "Chunk does not belong to this VirtualPageSource");

    MemoryChunk*& list = (chunk->size == m_chunk_size) ? m_free_chunks : m_free_spans;
    chunk->next = list;
    list = chunk;
}
