#pragma once

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Memory/LinearAllocator.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

namespace phx
{
    class FrameAllocator : public LinearAllocator 
    {
    public:
        PHX_NO_COPY_NO_MOVE(FrameAllocator);

        FrameAllocator() = default;
    };


    template<typename T>
    using FramePtr = T*;

    template<typename T>
    class FrameList
    {
    public:
        FrameList(FrameAllocator& alloc, u32 capacity)
            : m_data(alloc.AllocSpan<T>(capacity))
            , m_count(0)
        {};

        void Push(const T& d)
        {
            // TODO: Move?
            PHX_ASSERT(m_count < m_data.Size());
            m_data[m_count++] = d;
        }

        void Push(const T&& d)
        {
            // TODO: Move?
            PHX_ASSERT(m_count < m_data.Size());
            m_data[m_count++] = std::move(d);
        }

        Span<T> ToSpan() const
        {
            return Span<T>(m_data.data(), m_count);
        }
        
    private:
        SpanMutable<T> m_data;

        u32 m_count = 0;
    };
}