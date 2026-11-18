#pragma once

#include <PhxEngine/Core/PhxDefines.h>

namespace phx
{
    // Bump (linear) allocation over a range of offsets [0, capacity).
    //
    // Knows nothing about what the offsets point into: CPU memory, a mapped
    // GPU buffer, a heap. Concrete allocators pair it with a backing and turn
    // offsets into pointers/addresses. Not thread-safe.
    //
    // Alignment is applied to the offset, so the resulting address is only
    // aligned if the backing's base is at least as aligned.
    class BumpCore
    {
    public:
        static constexpr u64 kInvalidOffset = ~u64(0);

        using Mark = u64;

    public:
        BumpCore() = default;
        explicit BumpCore(u64 capacity) noexcept : m_capacity(capacity) {}

        // `used` starts the cursor part-way in (e.g. past a header that lives
        // in the same range), so alignment stays relative to the range base.
        void Initialize(u64 capacity, u64 used = 0) noexcept
        {
            PHX_ASSERT(used <= capacity);
            m_capacity = capacity;
            m_offset   = used;
        }

        // Returns the aligned offset of the allocation, or kInvalidOffset if
        // it doesn't fit. A zero-size allocation is allowed and consumes only
        // alignment padding.
        [[nodiscard]] u64 Alloc(u64 size, u64 alignment) noexcept
        {
            PHX_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0);

            const u64 aligned_offset = AlignUp(m_offset, alignment);
            if (aligned_offset > m_capacity || size > m_capacity - aligned_offset)
                return kInvalidOffset;

            m_offset = aligned_offset + size;
            return aligned_offset;
        }

        // Scoped rewind: everything allocated after GetMark() is released by
        // Rewind(mark).
        [[nodiscard]] Mark GetMark() const noexcept { return m_offset; }
        void Rewind(Mark mark) noexcept
        {
            PHX_ASSERT(mark <= m_offset && "Rewind past the current offset (stale or foreign mark)");
            m_offset = mark;
        }

        void Reset() noexcept { m_offset = 0; }

        [[nodiscard]] u64 Used()      const noexcept { return m_offset; }
        [[nodiscard]] u64 Remaining() const noexcept { return m_capacity - m_offset; }
        [[nodiscard]] u64 Capacity()  const noexcept { return m_capacity; }

    private:
        u64 m_offset   = 0;
        u64 m_capacity = 0;
    };
}
