#pragma once

#include <PhxEngine/Core/PhxDefines.h>

#include <mutex>

namespace phx
{
    // Header at the start of every chunk handed out by VirtualPageSource.
    // `next` is an intrusive link: a chunk is in exactly one list at a time
    // (the source's free list, a FrameArena generation list, or a
    // LinearAllocator's chunk chain), so no side allocations are needed.
    struct MemoryChunk
    {
        MemoryChunk* next = nullptr;
        u64          size = 0;     // total bytes, header included
    };

    // Allocations start this far into a chunk. Keeps the data start
    // cache-line aligned; the chunk base itself is page aligned.
    inline constexpr usize kChunkHeaderSize = 64;
    static_assert(sizeof(MemoryChunk) <= kChunkHeaderSize);

    // The global virtual-memory page source.
    //
    // Reserves one large address range up front and hands it out in chunks.
    // Every chunk is a multiple of the chunk size; a request larger than one
    // chunk gets a dedicated multi-chunk span. Released chunks go onto free
    // lists and stay committed -- the steady state does no syscalls and the
    // reservation never runs out from churn.
    //
    // Thread-safe. Acquire/Release take a mutex, which is fine: they're
    // the cold path (roughly once per chunk_size bytes allocated).
    class VirtualPageSource
    {
    public:
        struct Descriptor
        {
            usize reserve_size = 16_GB;
            usize chunk_size   = 256_KB;   // rounded up to the allocation granularity
        };

    public:
        PHX_NO_COPY_NO_MOVE(VirtualPageSource);

        VirtualPageSource() = default;
        ~VirtualPageSource() { Shutdown(); }

    public:
        bool Initialize(const Descriptor& desc);

        // Releases the whole reservation. Any chunk still held elsewhere
        // becomes invalid; releasing one afterwards is a no-op.
        void Shutdown();

        // Returns a chunk with at least `min_usable_bytes` after the header.
        // Running out of reservation is fatal.
        [[nodiscard]] MemoryChunk* AcquireChunk(usize min_usable_bytes);

        // Returns one chunk, or a whole `next`-linked list, to the free lists.
        void ReleaseChunk(MemoryChunk* chunk);
        void ReleaseChunkList(MemoryChunk* head);

        [[nodiscard]] bool  IsInitialized()  const { return m_base != nullptr; }
        [[nodiscard]] usize ChunkSize()      const { return m_chunk_size; }
        [[nodiscard]] usize ReservedBytes()  const { return m_reserved; }
        [[nodiscard]] usize CommittedBytes() const { std::scoped_lock lock(m_mutex); return m_committed; }

    private:
        void ReleaseLocked(MemoryChunk* chunk);

        u8*   m_base       = nullptr;
        usize m_reserved   = 0;
        usize m_carved     = 0;   // bytes handed out from the reservation so far
        usize m_committed  = 0;
        usize m_chunk_size = 0;

        MemoryChunk* m_free_chunks = nullptr;   // exactly chunk_size
        MemoryChunk* m_free_spans  = nullptr;   // larger, first-fit

        mutable std::mutex m_mutex;
    };
}
