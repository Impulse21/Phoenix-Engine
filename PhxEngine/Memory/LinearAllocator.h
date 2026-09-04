#pragma once

#include <PhxEngine/Memory/IAllocator.h>
#include <PhxEngine/Memory/VirtualMemoryArena.h>

namespace phx
{
    // Plain, lock-free bump allocator -- not safe to share across threads.
    // Meant to be used one instance per thread (see Memory::g_Frame /
    // g_Scratch, which are thread_local); any synchronization needed when
    // it grows into fresh pages lives in VirtualMemoryArena::Commit/Carve,
    // not here.
    class LinearAllocator : public IAllocator
    {
    public:
        PHX_NO_COPY_NO_MOVE(LinearAllocator);

        LinearAllocator() = default;

    public:
        void Initialize(void* block, usize block_size);
        void Initialize(VirtualMemoryArena* arena, void* base, usize reserveSize);
        void Shutdown();

        [[nodiscard]] void* Alloc(usize size, usize alignment = 8);
        void Free(void*) {};

        void Reset() { m_current = m_base; }

        [[nodiscard]] usize GetUsedBytes() const { return m_current - m_base; }
        [[nodiscard]] usize GetCommittedBytes() const { return m_committed - m_base; }
        [[nodiscard]] usize GetFreeBytes() const { return m_end - m_current; }
        [[nodiscard]] usize GetTotalBytes() const { return m_end - m_base; }

    private:
        bool GrowCommit(usize needed);

        VirtualMemoryArena* m_arena     = nullptr;
        u8*                 m_base      = nullptr;
        u8*                 m_current   = nullptr;
        u8*                 m_committed = nullptr;
        u8*                 m_end       = nullptr;
    };
}