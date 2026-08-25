#pragma once

#include "Handle.h"
#include "StaticArray.h"

#include <atomic>
#include <mutex>
#include <memory>

namespace phx
{
    template<class THandle, class TData, uint16_t MAX_SIZE>
    class SmallObjectPool
    {
        static_assert(MAX_SIZE <= 16, "SmallObjectPool is intended for small counts (<=16). Use PagedPool for larger allocations.");
        static_assert(MAX_SIZE > 0);

    public:
        SmallObjectPool()
        {
            for (uint16_t i = 0; i < MAX_SIZE; i++)
                m_generations[i] = 1;

            m_free_mask.store(FullMask(), std::memory_order_relaxed);
        }

        ~SmallObjectPool()
        {
            uint16_t alive = ~m_free_mask.load(std::memory_order_relaxed) & FullMask();
            while (alive)
            {
                uint16_t index = static_cast<uint16_t>(std::countr_zero(alive));
                m_data[index].~TData();
                alive &= alive - 1;
            }
        }

        PHX_NO_COPY_NO_MOVE(SmallObjectPool);
        
        void Shutdown()
        {
            uint16_t alive = ~m_free_mask.exchange(FullMask(), std::memory_order_acq_rel) & FullMask();
            while (alive)
            {
                uint16_t index = static_cast<uint16_t>(std::countr_zero(alive));
                uint16_t nextGen = m_generations[index] + 1;
                m_generations[index] = (nextGen == 0) ? 1 : nextGen;

                m_data[index].~TData();
                alive &= alive - 1;
            }
        }

        Handle<THandle> Allocate(TData*& out_data)
        {
            auto handle = Allocate();
            out_data = m_data + handle.m_index;
            return handle;
        }

        Handle<THandle> Allocate()
        {
            uint16_t current = m_free_mask.load(std::memory_order_relaxed);

            for (;;)
            {
                if (current == 0)
                    throw std::runtime_error("SmallObjectPool OOM!");

                uint16_t index  = static_cast<uint16_t>(std::countr_zero(current));
                uint16_t desired = current & ~(uint16_t(1) << index); // mark slot as used

                if (m_free_mask.compare_exchange_weak(current, desired,
                    std::memory_order_acquire,
                    std::memory_order_relaxed))
                {
                    new (m_data + index) TData();

                    return Handle<THandle>(index, m_generations[index]);
                }
            }
        }

        void Free(Handle<THandle> handle)
        {
            if (!Contains(handle))
                return;

            const uint16_t index = handle.m_index;

            m_data[index].~TData();

            const uint16_t nextGen = m_generations[index] + 1;
            m_generations[index] = (nextGen == 0) ? 1 : nextGen;
            
            m_free_mask.fetch_or(static_cast<uint16_t>(1u << index), std::memory_order_release);
        }

        TData* Get(Handle<THandle> handle)
        {
            if (!Contains(handle))
                return nullptr;

            return m_data + handle.m_index;
        }

        bool Contains(Handle<THandle> handle) const
        {
            if (!handle.IsValid() || handle.m_index >= MAX_SIZE)
                return false;

            // Check generation matches — stale handles to recycled slots are rejected
            if (m_generations[handle.m_index] != handle.m_generation)
                return false;

            // Confirm slot is actually alive in the bitmask
            uint16_t liveMask = ~m_free_mask.load(std::memory_order_acquire) & FullMask();
            return (liveMask >> handle.m_index) & 1;
        }

        constexpr u16 GetCapacity() const { return MAX_SIZE; }
        uint16_t GetCount() const
        {
            uint16_t mask = m_free_mask.load(std::memory_order_relaxed) & FullMask();
            uint16_t free = static_cast<u16>(std::popcount<uint16_t>(mask));
            return MAX_SIZE - free;
        }

        template<typename TFunc>
        void ForEach(TFunc&& func)
        {
            uint16_t alive = ~m_free_mask.load(std::memory_order_acquire) & FullMask();
            while (alive)
            {
                uint16_t index = static_cast<uint16_t>(std::countr_zero(alive));
                func(m_data[index]);
                alive &= alive - 1;
            }
        }

    private:
        static constexpr uint16_t FullMask()
        {
            return MAX_SIZE == 16 ? uint16_t(0xFFFF) : (1 << MAX_SIZE) - 1;
        }

    private:
        alignas(TData) unsigned char m_storage[sizeof(TData) * MAX_SIZE];

        // 2. Pointer alignment (8 bytes on 64-bit systems)
        TData* m_data = reinterpret_cast<TData*>(m_storage);

        // 3. Smallest alignment requirements (2 bytes each)
        std::atomic<uint16_t> m_free_mask;
        phx::StaticArray<uint16_t, MAX_SIZE> m_generations;

        uint8_t m_reserved_padding[6]; 
    };

    template<class THandle, class TData>
    class Pool
    {
    public:
        Pool() = default;
        ~Pool() { Shutdown(); }

        void Initialize(u32 max_handles)
        {
            m_max_handles = max_handles;

            // Integer Ceiling division.
            m_word_count = (max_handles + 31) / 32;

            m_alive_mask = std::make_unique<u32[]>(m_word_count); 

            m_data = std::make_unique<TData[]>(max_handles);
            m_free_list = std::make_unique<u16[]>(max_handles);
            m_generations = std::make_unique<u16[]>(max_handles);

            PHX_ASSERT(m_data && m_free_list && m_generations &&"Pool allocation failed!");
            
            // Initialize free list and generations
            for (u32 i = 0; i < max_handles; i++)
            {
                m_free_list[i]   = i;
                m_generations[i] = 1;
            }
        }

        void Shutdown()
        {
            if (!m_data)
                return;
            
            if constexpr(!std::is_trivially_destructible_v<TData>)
            {
                for (size_t i = 0; i < m_max_handles; ++i)
                {
                    if (IsAlive(i))
                    {
                        m_data[i].~TData();
                    }
                }
            }

            m_data              = nullptr;
            m_free_list         = nullptr;
            m_generations       = nullptr;
            m_alive_mask        = nullptr;
            m_free_list_head    = 0;
            m_word_count        = 0;
        }
    

        Handle<THandle> Allocate()
        {
            std::scoped_lock lock(m_allocation_mutex);

            if (m_free_list_head >= m_max_entries)
                throw std::runtime_error("Pool OOM (Handle limit hit)!");

            Handle<THandle> handle;
            handle.m_index = m_free_list[m_free_list_head++];
            handle.m_generation = m_generations[handle.m_index];

            SetIsAliveBit(handle.m_index);

            TData* address = &m_data[handle.m_index];
            ::new (static_cast<void*>(address)) TData();

            return handle;
        }

        void Free(Handle<THandle> handle)
        {
            if (!Contains(handle)) 
                return;


            if constexpr(!std::is_trivially_destructible_v<TData>)
            {
                Get(handle)->~TData();
            }

            {
                std::scoped_lock lock(m_allocation_mutex);
                m_generations[handle.m_index]++;

                // Don't let generation go back to zero as zero is reserved.
                if (m_generations[handle.m_index] == 0) 
                    m_generations[handle.m_index] = 1;

                m_free_list[--m_free_list_head] = handle.m_index;
                ClearIsAliveBit(handle.m_index);
            }
        }

        // --- Getters ---
        TData* Get(Handle<THandle> handle)
        {
            if (!Contains(handle)) 
                return nullptr;

            return &m_data[handle.m_index];
        }

        TData* Get(uint16_t index, uint16_t generation)
        {
            Handle<THandle> h(index, generation);
            return Get(h);
        }

        bool Contains(Handle<THandle> handle) const
        {
            return handle.IsValid() &&
                m_generations[handle.m_index] == handle.m_generation;
        }
        
        bool IsEmpty() const { return m_free_list_head == 0; }

        template<typename TFunc>
        void ForEach(TFunc&& func)
        {
            for (u32 w = 0; w < m_word_count; w++)
            {
                u32 word = m_alive_mask[w];
                while (word)
                {
                    u32 bit   = std::countr_zero(word);
                    u32 index = w * 32 + bit;
                    func(m_data[index]);
                    word &= word - 1;
                }
            }
        }

    private:
        bool IsAlive(u32 index) const
        {
            PHX_ASSERT(index < m_max_handles && "Index out of bounds in Pool::IsAlive"); 
            return (m_alive_mask[index / 32] >> (index % 32)) & 1u;
        }

        void SetIsAliveBit(u32 index)
        {
            m_alive_mask[index / 32] |= (1u << index % 32);
        }

        void ClearIsAliveBit(u32 index)
        {
            m_alive_mask[index / 32] &= ~(1u << index % 32);
        }

    private:
        std::mutex                      m_allocation_mutex;
        u32                             m_max_handles       = 0u;
        usize                           m_max_entries       = 0u;

        usize                           m_free_list_head    = 0;

        std::unique_ptr<TData[]>        m_data          = nullptr;
        std::unique_ptr<u16[]>          m_free_list     = nullptr;
        std::unique_ptr<u16[]>          m_generations   = nullptr;
        std::unique_ptr<u32[]>          m_alive_mask    = nullptr;
        u32                             m_word_count    = 0;     
    };
}