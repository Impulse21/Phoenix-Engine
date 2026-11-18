#pragma once

#include <PhxEngine/Memory/VirtualPageSource.h>

#include <atomic>
#include <mutex>

namespace phx
{
    // The per-frame arena: tracks which page-source chunks belong to which
    // frame ("generation") and releases them all at the frame boundary.
    //
    // Per-thread FrameAllocators bump through chunks acquired here; the
    // arena -- not the threads -- reclaims them, so frame memory's lifetime is
    // the frame no matter which thread allocated it or where the pointer went.
    //
    // A chunk acquired during generation G is released by the EndFrame() call
    // that ends generation G + kFrameLatency - 1. With kFrameLatency = 1, frame
    // memory is valid until the EndFrame() of the frame that allocated it.
    class FrameArena
    {
    public:
        static constexpr u32 kFrameLatency = 1;

        struct FrameChunk
        {
            MemoryChunk* chunk      = nullptr;
            u64          generation = 0;
        };

    public:
        PHX_NO_COPY_NO_MOVE(FrameArena);

        FrameArena() = default;
        ~FrameArena() { Shutdown(); }

    public:
        void Initialize(VirtualPageSource& source);

        // Releases every generation's chunks back to the source.
        void Shutdown();

        // Thread-safe. Returns a chunk registered to the current generation,
        // tagged with that generation.
        [[nodiscard]] FrameChunk AcquireChunk(usize min_usable_bytes);

        // Ends the current generation: releases the chunks of the oldest live
        // generation and advances. The caller guarantees nothing is still
        // allocating from, or reading, the memory being released -- i.e. call
        // it after every job that used frame memory this frame has finished.
        void EndFrame();

        // Read on every FrameAllocator::Alloc. Relaxed is enough: a thread
        // only allocates after the job system has synchronized it with the
        // EndFrame() that advanced the generation.
        [[nodiscard]] u64 CurrentGeneration() const { return m_generation.load(std::memory_order_relaxed); }

        [[nodiscard]] bool IsInitialized() const { return m_source != nullptr; }

    private:
        VirtualPageSource* m_source = nullptr;
        std::atomic<u64>   m_generation{ 0 };

        // m_live[g % kFrameLatency] heads generation g's intrusive chunk list.
        MemoryChunk* m_live[kFrameLatency] = {};
        std::mutex   m_mutex;
    };
}
