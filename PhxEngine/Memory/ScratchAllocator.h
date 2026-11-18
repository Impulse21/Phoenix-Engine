#pragma once

#include <PhxEngine/Memory/LinearAllocator.h>

namespace phx
{
    // A thread's scratch memory: short-lived temporaries released by scope
    // (ScratchScope), not by frame. Chunks are kept by the thread as spares
    // once acquired, so steady-state use takes no locks.
    // Get the calling thread's with Memory::GetScratchAlloc() or open a scope
    // with Memory::Scratch(); never hand one to another thread.
    class ScratchAllocator : public LinearAllocator
    {
    public:
        PHX_NO_COPY_NO_MOVE(ScratchAllocator);
        ScratchAllocator() = default;
    };

    // RAII scratch region: everything allocated through the allocator after
    // the scope opened is released when it closes. Scopes nest (strict LIFO).
    //
    //     ScratchScope scratch = Memory::Scratch();
    //     SpanMutable<u32> tmp = scratch.AllocSpan<u32>(n);
    class ScratchScope
    {
    public:
        PHX_NO_COPY_NO_MOVE(ScratchScope);

        explicit ScratchScope(ScratchAllocator& allocator)
            : m_allocator(&allocator)
            , m_mark(allocator.GetMark())
        {}

        ~ScratchScope() { m_allocator->Rewind(m_mark); }

        [[nodiscard]] void* Alloc(usize size, usize alignment = alignof(std::max_align_t))
        {
            return m_allocator->Alloc(size, alignment);
        }

        template<typename T>
        [[nodiscard]] T* Alloc(usize count = 1, usize alignment = alignof(T))
        {
            return m_allocator->Alloc<T>(count, alignment);
        }

        template<typename T>
        [[nodiscard]] SpanMutable<T> AllocSpan(usize count, usize alignment = alignof(T))
        {
            return m_allocator->AllocSpan<T>(count, alignment);
        }

        // For APIs that take a LinearAllocator& (e.g. BitSet::Create).
        [[nodiscard]] ScratchAllocator& Get() { return *m_allocator; }

    private:
        ScratchAllocator*      m_allocator;
        LinearAllocator::Mark  m_mark;
    };
}
