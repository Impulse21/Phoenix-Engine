#pragma once

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Memory/BumpCore.h>
#include <PhxEngine/Memory/FrameArena.h>
#include <PhxEngine/Memory/VirtualPageSource.h>

#include <cstddef>
#include <new>
#include <type_traits>

#if defined(PHX_DEBUG)
#include <thread>
#endif

namespace phx
{
    // Bump allocator: a cursor (BumpCore) over the current chunk of memory.
    // The hot path is a few adds and a compare -- no locks, no atomics RMW.
    //
    // What happens when the cursor runs out depends on the backing:
    //   - Fixed block:   never grows; overflow is fatal.
    //   - Page source:   acquires another chunk and owns it. Rewind/Reset keep
    //                    chunks as spares for reuse; Shutdown returns them.
    //                    (ScratchAllocator)
    //   - Frame arena:   acquires a chunk filed under the current frame; the
    //                    arena releases it at the frame boundary. When the
    //                    frame changes, the cursor abandons its chunk and
    //                    grabs a fresh one. (FrameAllocator)
    //
    // Not thread-safe: one allocator per thread. Destructors of allocated
    // objects never run.
    class LinearAllocator
    {
    public:
        // Chunk bases are page aligned (>= 4 KB on every supported platform).
        static constexpr usize kMaxAlignment = 4096;

        // Position to Rewind() back to. Only valid on the allocator that made it.
        struct Mark
        {
            MemoryChunk* chunk  = nullptr;
            u64          offset = 0;
        };

    public:
        PHX_NO_COPY_NO_MOVE(LinearAllocator);

        LinearAllocator() = default;
        ~LinearAllocator() { Shutdown(); }

    public:
        void Initialize(void* block, usize block_size);
        void Initialize(VirtualPageSource& source);
        void Initialize(FrameArena& arena);

        // Returns owned chunks (page-source backing) and detaches.
        void Shutdown();

        [[nodiscard]] bool IsInitialized() const { return m_backing != Backing::None; }

        // Debug: assert every later allocation comes from the calling thread.
        void BindToCurrentThread();

        // Raw, uninitialized bytes. Running out of memory is fatal (logs and
        // aborts) -- this never returns nullptr.
        [[nodiscard]] void* Alloc(usize size, usize alignment = alignof(std::max_align_t))
        {
            PHX_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0 && alignment <= kMaxAlignment);
            DebugCheckOwnerThread();

            // A frame-backed cursor is only usable during the generation its
            // chunk was filed under.
            if (m_frame_arena == nullptr || m_frame_arena->CurrentGeneration() == m_generation)
            {
                const u64 offset = m_core.Alloc(size, alignment);
                if (offset != BumpCore::kInvalidOffset)
                    return m_base + offset;
            }

            return AllocSlow(size, alignment);
        }

        // Default-initializes each element: a no-op for trivial types, runs
        // default member initializers otherwise. Destructors never run (the
        // memory is reclaimed in bulk), hence the trivially-destructible
        // requirement.
        template<typename T>
        [[nodiscard]] T* Alloc(usize count = 1, usize alignment = alignof(T))
        {
            static_assert(std::is_trivially_destructible_v<T>,
                "LinearAllocator never runs destructors; T must be trivially destructible");

            PHX_ASSERT(alignment >= alignof(T));
            T* ptr = static_cast<T*>(Alloc(sizeof(T) * count, alignment));
            for (usize i = 0; i < count; ++i)
                ::new (static_cast<void*>(ptr + i)) T;

            return ptr;
        }

        template<typename T>
        [[nodiscard]] SpanMutable<T> AllocSpan(usize count, usize alignment = alignof(T))
        {
            T* ptr = Alloc<T>(count, alignment);
            return SpanMutable<T>(ptr, count);
        }

        // Fixed-block and page-source backings only: frame memory is
        // reclaimed by the FrameArena, not rewound.
        [[nodiscard]] Mark GetMark() const;
        void Rewind(Mark mark);
        void Reset();

    private:
        enum class Backing : u8 { None, FixedBlock, PageSource, FrameArena };

        void* AllocSlow(usize size, usize alignment);
        void  SetCursor(MemoryChunk* chunk, u64 used);
        static bool Fits(const MemoryChunk* chunk, usize size, usize alignment);

        void DebugCheckOwnerThread() const
        {
#if defined(PHX_DEBUG)
            PHX_ASSERT(m_owner == std::thread::id{} || m_owner == std::this_thread::get_id());
#endif
        }

        // -- Cursor (hot) ---
        u8*         m_base        = nullptr;   // base of the current chunk / block
        BumpCore    m_core;
        FrameArena* m_frame_arena = nullptr;   // non-null for the frame backing
        u64         m_generation  = ~u64(0);   // generation of the current frame chunk

        // -- Backing (cold) ---
        Backing            m_backing = Backing::None;
        MemoryChunk*       m_chunk   = nullptr;   // current chunk (null for fixed block)
        MemoryChunk*       m_first   = nullptr;   // page source: head of the owned chain
        VirtualPageSource* m_source  = nullptr;

#if defined(PHX_DEBUG)
        std::thread::id m_owner;
#endif
    };
}
