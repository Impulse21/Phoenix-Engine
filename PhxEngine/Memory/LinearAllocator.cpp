#include "LinearAllocator.h"

#include <PhxEngine/Core/Log.h>

#include <cstdlib>

namespace phx
{
    void LinearAllocator::Initialize(void* block, usize block_size)
    {
        PHX_ASSERT(m_backing == Backing::None && "LinearAllocator initialized twice");
        m_backing = Backing::FixedBlock;
        m_base    = static_cast<u8*>(block);
        m_core.Initialize(block_size);
    }

    void LinearAllocator::Initialize(VirtualPageSource& source)
    {
        PHX_ASSERT(m_backing == Backing::None && "LinearAllocator initialized twice");
        m_backing = Backing::PageSource;
        m_source  = &source;
        m_core.Initialize(0);   // first Alloc acquires a chunk
    }

    void LinearAllocator::Initialize(FrameArena& arena)
    {
        PHX_ASSERT(m_backing == Backing::None && "LinearAllocator initialized twice");
        m_backing     = Backing::FrameArena;
        m_frame_arena = &arena;
        m_core.Initialize(0);   // first Alloc acquires a chunk
    }

    void LinearAllocator::Shutdown()
    {
        if (m_backing == Backing::PageSource && m_source != nullptr)
            m_source->ReleaseChunkList(m_first);

        // Frame chunks belong to the FrameArena; fixed blocks to the caller.

        m_base        = nullptr;
        m_core        = {};
        m_frame_arena = nullptr;
        m_generation  = ~u64(0);
        m_backing     = Backing::None;
        m_chunk       = nullptr;
        m_first       = nullptr;
        m_source      = nullptr;
#if defined(PHX_DEBUG)
        m_owner       = {};
#endif
    }

    void LinearAllocator::BindToCurrentThread()
    {
#if defined(PHX_DEBUG)
        m_owner = std::this_thread::get_id();
#endif
    }

    bool LinearAllocator::Fits(const MemoryChunk* chunk, usize size, usize alignment)
    {
        const u64 start = AlignUp(static_cast<u64>(kChunkHeaderSize), static_cast<u64>(alignment));
        return start <= chunk->size && size <= chunk->size - start;
    }

    void LinearAllocator::SetCursor(MemoryChunk* chunk, u64 used)
    {
        m_chunk = chunk;
        m_base  = reinterpret_cast<u8*>(chunk);
        m_core.Initialize(chunk->size, used);
    }

    void* LinearAllocator::AllocSlow(usize size, usize alignment)
    {
        // Worst-case padding in a fresh chunk is alignment - 1 past the header.
        const usize min_usable = size + alignment;

        switch (m_backing)
        {
            case Backing::FixedBlock:
            {
                PHX_LOG_ERROR(Log::Channels::Memory,
                    "LinearAllocator out of space: requested {0} bytes (align {1}), {2}/{3} bytes used",
                    size, alignment, m_core.Used(), m_core.Capacity());
                PHX_ASSERT(false);
                std::abort();
            }

            case Backing::PageSource:
            {
                // Reuse the spare chunk after the cursor (left by Rewind/Reset)
                // if it's big enough; otherwise splice a new one in before it.
                MemoryChunk* spare = (m_chunk != nullptr) ? m_chunk->next : m_first;
                if (spare != nullptr && Fits(spare, size, alignment))
                {
                    SetCursor(spare, kChunkHeaderSize);
                    break;
                }

                MemoryChunk* fresh = m_source->AcquireChunk(min_usable);
                fresh->next = spare;
                if (m_chunk != nullptr)
                    m_chunk->next = fresh;
                else
                    m_first = fresh;

                SetCursor(fresh, kChunkHeaderSize);
                break;
            }

            case Backing::FrameArena:
            {
                // Either this chunk is full or the frame changed under it. The
                // old chunk is the arena's to release; just move on.
                const FrameArena::FrameChunk frame_chunk = m_frame_arena->AcquireChunk(min_usable);
                m_generation = frame_chunk.generation;
                SetCursor(frame_chunk.chunk, kChunkHeaderSize);
                break;
            }

            case Backing::None:
            default:
                PHX_LOG_ERROR(Log::Channels::Memory, "LinearAllocator used before Initialize");
                PHX_ASSERT(false);
                std::abort();
        }

        const u64 offset = m_core.Alloc(size, alignment);
        PHX_ASSERT(offset != BumpCore::kInvalidOffset);
        return m_base + offset;
    }

    LinearAllocator::Mark LinearAllocator::GetMark() const
    {
        PHX_ASSERT(m_backing == Backing::FixedBlock || m_backing == Backing::PageSource);
        return { .chunk = m_chunk, .offset = m_core.GetMark() };
    }

    void LinearAllocator::Rewind(Mark mark)
    {
        PHX_ASSERT(m_backing == Backing::FixedBlock || m_backing == Backing::PageSource);

        if (mark.chunk == m_chunk)
        {
            m_core.Rewind(mark.offset);
            return;
        }

#if defined(PHX_DEBUG)
        // The mark must be behind the cursor: its chunk comes before ours in
        // the chain (null = before the first chunk).
        bool found = (mark.chunk == nullptr);
        if (m_chunk != nullptr)
        {
            for (MemoryChunk* c = m_first; c != nullptr && c != m_chunk && !found; c = c->next)
                found = (c == mark.chunk);
        }
        PHX_ASSERT(found && "Rewind to a mark that is ahead of the cursor or from another allocator");
#endif

        // Chunks after the mark stay linked as spares.
        if (mark.chunk == nullptr)
        {
            m_chunk = nullptr;
            m_base  = nullptr;
            m_core.Initialize(0);
            return;
        }

        SetCursor(mark.chunk, mark.offset);
    }

    void LinearAllocator::Reset()
    {
        switch (m_backing)
        {
            case Backing::FixedBlock: m_core.Reset(); break;
            case Backing::PageSource: Rewind({}); break;
            default:
                PHX_ASSERT(false && "Reset: frame memory is reclaimed by the FrameArena");
                break;
        }
    }
}
