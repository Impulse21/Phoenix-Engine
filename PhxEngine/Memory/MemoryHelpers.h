#pragma once

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Memory/Memory.h>

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

// Convenience wrappers over the calling thread's frame / scratch allocators,
// so callers never hold (or leak to another thread) an allocator reference.
namespace phx::Memory
{
    // Frame memory: valid until EndFrame() of the current frame.

    [[nodiscard]] inline void* FrameAlloc(usize size, usize alignment = alignof(std::max_align_t))
    {
        return GetFrameAlloc().Alloc(size, alignment);
    }

    // Constructs one T from `args` (parenthesized init, so aggregates work).
    template<typename T, typename... Args>
    [[nodiscard]] T* FrameNew(Args&&... args)
    {
        static_assert(std::is_trivially_destructible_v<T>,
            "Frame memory never runs destructors; T must be trivially destructible");

        void* ptr = FrameAlloc(sizeof(T), alignof(T));
        return ::new (ptr) T(std::forward<Args>(args)...);
    }

    // `count` value-initialized Ts (zeroed for trivial types).
    template<typename T>
    [[nodiscard]] SpanMutable<T> FrameArray(usize count)
    {
        static_assert(std::is_trivially_destructible_v<T>,
            "Frame memory never runs destructors; T must be trivially destructible");

        T* ptr = static_cast<T*>(FrameAlloc(sizeof(T) * count, alignof(T)));
        for (usize i = 0; i < count; ++i)
            ::new (static_cast<void*>(ptr + i)) T();

        return SpanMutable<T>(ptr, count);
    }

    // Scratch memory: valid until the returned scope closes.
    [[nodiscard]] inline ScratchScope Scratch()
    {
        return ScratchScope(GetScratchAlloc());
    }
}

namespace phx
{
    // Fixed-capacity list in the calling thread's frame memory; valid until
    // Memory::EndFrame().
    template<typename T>
    class FrameList
    {
    public:
        explicit FrameList(u32 capacity)
            : m_data(Memory::GetFrameAlloc().AllocSpan<T>(capacity))
            , m_count(0)
        {}

        // Copy/move-constructs into the slot rather than assigning, so T's
        // lifetime is begun properly even for non-trivial T.
        void Push(const T& d)
        {
            PHX_ASSERT(m_count < m_data.Size());
            ::new (static_cast<void*>(&m_data[m_count++])) T(d);
        }

        void Push(T&& d)
        {
            PHX_ASSERT(m_count < m_data.Size());
            ::new (static_cast<void*>(&m_data[m_count++])) T(std::move(d));
        }

        u32 GetSize() const { return m_count; }
        
        Span<T> ToSpan() const
        {
            return Span<T>(m_data.data(), m_count);
        }
        
    private:
        SpanMutable<T> m_data;

        u32 m_count = 0;
    };
}
