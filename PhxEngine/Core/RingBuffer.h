#pragma once

#include <mutex>

namespace phx
{
    template<typename T, u32 Capacity>
    class RingBuffer
    {
    public:
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of two");

        RingBuffer()  = default;
        ~RingBuffer() { Clear(); }

        PHX_NO_COPY_NO_MOVE(RingBuffer);

        bool Push(T&& item)
        {
            std::scoped_lock _(m_mutex);

            if (m_size >= Capacity)
                return false;

            u32 slot = (m_head + m_size) & (Capacity - 1);
            new (&m_storage[slot]) T(std::move(item));

            m_size++;

            return true;
        }

        bool Pop(T& out)
        {
            std::scoped_lock _(m_mutex);

            if (m_size == 0) 
                return false;

            T* ptr = reinterpret_cast<T*>(&m_storage[m_head]);

            out = std::move(*ptr);

            ptr->~T();

            m_head = (m_head + 1) & (Capacity - 1);

            m_size--;
            
            return true;
        }

        [[nodiscard]] bool IsEmpty() const
        {
            std::scoped_lock _(m_mutex);
            return m_size == 0;
        }

        [[nodiscard]] u32 Size() const
        {
            std::scoped_lock _(m_mutex);
            return m_size;
        }

    private:
        void Clear()
        {
            T dummy;
            while (Pop(dummy)) {}
        }

    private:
        mutable std::mutex m_mutex;
        alignas(T) std::byte m_storage[Capacity * sizeof(T)];
        u32 m_head = 0;
        u32 m_size = 0;
    };
}