#pragma once

#include "Handle.h"

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
                uint16_t index = std::countr_zero(alive);
                m_data[index].~TData();
                alive &= alive - 1;
            }
        }

        PHX_NO_COPY_NO_MOVE(SmallObjectPool)
        
        void Shutdown()
        {
            uint16_t alive = ~m_free_mask.exchange(FullMask(), std::memory_order_acq_rel) & FullMask();
            while (alive)
            {
                uint16_t index = std::countr_zero(alive);
                uint16_t nextGen = m_generations[index] + 1;
                m_generations[index] = (nextGen == 0) ? 1 : nextGen;

                m_data[index].~TData();
                alive &= alive - 1;
            }
        }

        Handle<THandle> Allocate()
        {
            uint16_t current = m_free_mask.load(std::memory_order_relaxed);

            for (;;)
            {
                if (current == 0)
                    throw std::runtime_error("SmallObjectPool OOM!");

                uint16_t index  = std::countr_zero(current);
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
            
            m_free_mask.fetch_or(uint16_t(1) << index, std::memory_order_release);
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

        uint16_t GetCount() const
        {
            uint16_t mask = m_free_mask.load(std::memory_order_relaxed) & FullMask();
            uint16_t free = std::popcount<uint16_t>(mask);
            return MAX_SIZE - free;
        }

        template<typename TFunc>
        void ForEach(TFunc&& func)
        {
            uint16_t alive = ~m_free_mask.load(std::memory_order_acquire) & FullMask();
            while (alive)
            {
                uint16_t index = std::countr_zero(alive);
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
        std::atomic<uint16_t> m_free_mask;
        phx::StaticArray<uint16_t, MAX_SIZE> m_generations;

        // This is to avoid default consturction of TData elements, as they may be non-trivial.
        alignas(TData) unsigned char m_storage[sizeof(TData) * MAX_SIZE];
        TData* m_data = reinterpret_cast<TData*>(m_storage);
    };


    #if false
    static constexpr size_t kPageSize = 4096;

    template<class THandle, class TDataHot, class TDataCold = std::monostate>
    class PagedPool
    {
        static constexpr bool HasColdData = !std::is_same_v<TDataCold, std::monostate>;

    public:
        PagedPool() = default;
        ~PagedPool() { Shutdown(); }

        void Initialize(uint16_t max_handles)
        {
            m_max_entries = max_handles;
            m_data_hot = Platform::VirtualMemReserveTyped<TDataHot, kPageSize>(m_max_entries);
            m_free_list = Platform::VirtualMemReserveTyped<uint16_t, kPageSize>(m_max_entries);
            m_generations = Platform::VirtualMemReserveTyped<uint16_t, kPageSize>(m_max_entries);

            if constexpr (HasColdData)
            {
                m_data_cold = Platform::VirtualMemReserveTyped<TDataCold, kPageSize>(m_max_entries);
                if (!m_data_cold) 
                    throw std::runtime_error("Failed to reserve cold pool.");
            }

            if (!m_data_hot || !m_free_list || !m_generations)
                throw std::runtime_error("Failed to reserve hot pool.");

            EnsureCapacity(1);
        }

        void Shutdown()
        {
            std::vector<bool> is_alive(m_committed_indices, true);
            if (m_free_list)
            {
                for (size_t i = m_free_list_head; i < m_committed_indices; ++i)
                {
                    uint32_t free_idx = m_free_list[i];
                    if (free_idx < m_committed_indices)
                    {
                        is_alive[free_idx] = false;
                    }
                }
            }

            if (m_data_hot)
            {
                for (size_t i = 0; i < m_committed_indices; i++)
                {
                    if (is_alive[i])
                    {
                        m_data_hot[i].~TDataHot();
                    }
                }
                Platform::VirtualMemFree(m_data_hot);
                m_data_hot = nullptr;
            }

            if constexpr (HasColdData)
            {
                if (m_data_cold)
                {
                    if constexpr (!std::is_trivially_destructible_v<TDataCold>)
                    {
                        for (size_t i = 0; i < m_committed_indices; i++)
                        {
                            if (is_alive[i])
                            {
                                m_data_cold[i].~TDataCold();
                            }
                        }
                    }
                    Platform::VirtualMemFree(m_data_cold);
                    m_data_cold = nullptr;
                }
            }

            if (m_free_list) 
            { 
                Platform::VirtualMemFree(m_free_list); 
                m_free_list = nullptr; 
            }

            if (m_generations) 
            { 
                Platform::VirtualMemFree(m_generations); 
                m_generations = nullptr; 
            }
        }

        Handle<THandle> Allocate()
        {
            std::scoped_lock lock(m_allocation_mutex);

            if (m_free_list_head >= m_max_entries)
                throw std::runtime_error("Pool OOM (Handle limit hit)!");

            if (m_free_list_head >= m_committed_indices)
                EnsureCapacity(m_committed_indices + 1);

            Handle<THandle> handle;
            handle.m_index = m_free_list[m_free_list_head++];
            handle.m_generation = m_generations[handle.m_index];

            new (this->m_data_hot + handle.m_index) TDataHot();

            if constexpr (HasColdData)
                new (this->m_data_cold + handle.m_index) TDataCold();

            return handle;
        }

        void Free(Handle<THandle> handle)
        {
            if (!Contains(handle)) 
                return;

            GetHot(handle)->~TDataHot();
            if constexpr (HasColdData && !std::is_trivially_destructible_v<TDataCold>)
                GetCold(handle)->~TDataCold();

            {
                std::scoped_lock lock(m_allocation_mutex);
                m_generations[handle.m_index]++;
                if (m_generations[handle.m_index] == 0) m_generations[handle.m_index] = 1;
                m_free_list[--m_free_list_head] = handle.m_index;
            }
        }

        // --- Getters ---
        TDataHot* GetHot(Handle<THandle> handle)
        {
            if (!Contains(handle)) 
                return nullptr;

            return m_data_hot + handle.m_index;
        }

        TDataHot* GetHot(uint16_t index, uint16_t generation)
        {
            Handle<THandle> h(index, generation);
            return GetHot(h);
        }

        TDataCold* GetCold(Handle<THandle> handle)
        {
            if constexpr (!HasColdData) 
                return nullptr;

            if (!Contains(handle)) 
                return nullptr;

            return m_data_cold + handle.m_index;
        }

        TDataCold* GetCold(uint16_t index, uint16_t generation)
        {
            if constexpr (!HasColdData) 
                return nullptr;

            if (index >= m_committed_indices || m_generations[index] != generation) 
                return nullptr;

            return m_data_cold + index;
        }

        TDataHot* GetDataHot() { return m_data_hot; }
        const uint16_t* GetGenerations() const { return m_generations; }
        size_t GetCommittedSize() const { return m_committed_indices; }

        bool Contains(Handle<THandle> handle) const
        {
            return handle.IsValid() &&
                handle.m_index < m_committed_indices &&
                m_generations[handle.m_index] == handle.m_generation;
        }
        bool IsEmpty() const { return m_free_list_head == 0; }

    private:
        void EnsureCapacity(size_t capacity_needed)
        {
            size_t target_index = std::min((size_t)m_max_entries, capacity_needed);
            if (target_index <= m_committed_indices) return;

            auto CommitStream = [&](void* base_ptr, size_t element_size)
                {
                    if (!base_ptr) 
                        return;

                    size_t current_bytes = m_committed_indices * element_size;
                    size_t target_bytes = target_index * element_size;

                    size_t page_curr = (current_bytes + kPageSize - 1) & ~(kPageSize - 1);
                    size_t page_target = (target_bytes + kPageSize - 1) & ~(kPageSize - 1);

                    if (page_target > page_curr)
                    {
                        size_t bytes_to_commit = page_target - page_curr;
                        Platform::VirtualMemCommit((uint8_t*)base_ptr + page_curr, bytes_to_commit);
                    }
                };

            CommitStream(m_data_hot, sizeof(TDataHot));

            if constexpr (HasColdData)
                CommitStream(m_data_cold, sizeof(TDataCold));

            CommitStream(m_free_list, sizeof(uint16_t));
            CommitStream(m_generations, sizeof(uint16_t));

            for (size_t i = m_committed_indices; i < target_index; ++i)
            {
                m_free_list[i] = (uint16_t)i;
                m_generations[i] = 1;
            }

            m_committed_indices = target_index;
        }

    private:
        size_t     m_max_entries;
        size_t     m_committed_indices = 0;
        size_t     m_free_list_head = 0;

        TDataHot* m_data_hot = nullptr;
        TDataCold* m_data_cold = nullptr;
        uint16_t* m_free_list = nullptr;
        uint16_t* m_generations = nullptr;

        std::mutex m_allocation_mutex;
    };
#endif
}