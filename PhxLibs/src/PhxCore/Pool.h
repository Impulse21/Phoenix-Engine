#pragma once

#include <PhxCore/Platform/PlatformWrapper.h>
#include <PhxEngine/Core/Handle.h>
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace phx
{
    static constexpr size_t kPageSize = 4096;

    template<class THandle, class TDataHot, class TDataCold = std::monostate>
    class PagedPool
    {
        static_assert(std::is_default_constructible_v<TDataHot>);
        static_assert(std::is_default_constructible_v<TDataCold>, "TDataCold should have a trivial destructor");

    public:
        PagedPool() = default;
        ~PagedPool()
        {
            Shutdown();
        }

        void Initialize(uint16_t maxHandles, uint16_t grow_size = 128)
        {
            m_max_entries = std::min(maxHandles, std::numeric_limits<uint16_t>::max());
            m_grow_size = grow_size;

            m_data_hot = Platform::VirtualMemReserve<TDataHot>(m_max_entries);
            m_data_cold = Platform::VirtualMemReserve<TDataCold>(m_max_entries);
            m_free_list = Platform::VirtualMemReserve<uint16_t>(m_max_entries);
            m_generations = Platform::VirtualMemReserve<uint16_t>(m_max_entries);

            if (!m_data_hot || !m_free_list || !m_generations)
                throw std::runtime_error("Failed to reserve virtual pool memory.");

            EnsureCapacity(m_grow_size);
        }

        void Finalize()
        {
            Shutdown();
        }

        void Shutdown()
        {
            if (m_data_hot)
            {
                for (size_t i = 0; i < m_committed_indices; i++)
                {
                    m_data_hot[i].~TDataHot();
                }

                Platform::Get().VirtualMemFree(m_data_hot);
                m_data_hot = nullptr;
            }

            if (m_data_cold)
            {
                if constexpr (!std::is_same_v<TDataCold, std::monostate>)
                {
                    for (size_t i = 0; i < m_committed_indices; i++)
                    {
                        m_data_cold[i].~TDataCold();
                    }
                }
                Platform::Get().VirtualMemFree(m_data_cold);
                m_data_cold = nullptr;
            }

            if (m_free_list)
            {
                Platform::Get().VirtualMemFree(m_free_list);
                m_free_list = nullptr;
            }

            if (m_generations)
            {
                Platform::Get().VirtualMemFree(m_generations);
                m_generations = nullptr;
            }
        }

        Handle<THandle> Allocate()
        {
            if (m_free_list_head >= m_max_entries)
                throw std::runtime_error("Pool is out of memory (Max Virtual Reserve hit)!");

            if (m_free_list_head >= m_committed_indices)
                EnsureCapacity(m_committed_indices + m_grow_size);

            Handle<THandle> handle;
            handle.m_index = m_free_list[m_free_list_head++];
            handle.m_generation = m_generations[handle.m_index];

            new (this->m_data_hot + handle.m_index) TDataHot();
            if constexpr (!std::is_same_v<TDataCold, std::monostate>)
                new (this->m_data_cold + handle.m_index) TDataCold();

            return handle;
        }

        void Free(Handle<THandle> handle)
        {
            if (!Contains(handle))
            {
                return;
            }

            GetHot(handle)->~TDataHot();
            if constexpr (!std::is_same_v<TDataCold, std::monostate>)
                if constexpr (!std::is_trivially_destructible_v<TDataCold>)
                    GetCold(handle)->~TDataCold();

            m_data_hot[handle.m_index] = {};

            m_generations[handle.m_index]++;

            if (m_generations[handle.m_index] == std::numeric_limits<uint16_t>::max())
            {
                return;
            }
            if (m_generations[handle.m_index] == 0)
            {
                m_generations[handle.m_index] = 1;
            }

            m_free_list[--m_free_list_head] = handle.m_index;
        }

        template<typename T>
        T* Get(Handle<THandle> handle)
        {
            if constexpr (std::is_same_v<T, TDataHot>)
            {
                return GetHot(handle);
            }
            else if constexpr (std::is_same_v<T, TDataCold>)
            {
                return GetCold(handle);
            }
            else
            {
                static_assert(always_false<T>, "Unsupported handle type!");
            }
        }

        template<typename T>
        const T* Get(Handle<THandle> handle) const
        {
            if constexpr (std::is_same_v<T, TDataHot>)
            {
                return GetHot(handle);
            }
            else if constexpr (std::is_same_v<T, TDataCold>)
            {
                return GetCold(handle);
            }
            else
            {
                static_assert(always_false<T>, "Unsupported handle type!");
            }
        }

        TDataHot* GetHot(Handle<THandle> handle)
        {
            if (!Contains(handle)) return nullptr;
            return m_data_hot + handle.m_index;
        }

        const TDataHot* GetHot(Handle<THandle> handle) const
        {
            if (!Contains(handle)) return nullptr;
            return m_data_hot + handle.m_index;
        }

        TDataCold* GetCold(Handle<THandle> handle)
        {
            if (!Contains(handle)) return nullptr;
            return m_data_cold + handle.m_index;
        }

        const TDataCold* GetCold(Handle<THandle> handle) const
        {
            if (!Contains(handle)) return nullptr;
            return m_data_cold + handle.m_index;
        }

        bool Contains(Handle<THandle> handle) const
        {
            return
                handle.IsValid() &&
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
                    if (!base_ptr) return;

                    size_t current_bytes = m_committed_indices * element_size;
                    size_t target_bytes = target_index * element_size;

                    size_t page_curr = (current_bytes + kPageSize - 1) & ~(kPageSize - 1);
                    size_t page_target = (target_bytes + kPageSize - 1) & ~(kPageSize - 1);

                    if (page_target > page_curr)
                    {
                        size_t bytes_to_commit = page_target - page_curr;
                        Platform::Get().VirtualMemCommit((uint8_t*)base_ptr + page_curr, bytes_to_commit);
                    }
                };

            CommitStream(m_data_hot, sizeof(TDataHot));
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
        size_t     m_grow_size = 128;

        TDataHot* m_data_hot = nullptr;
        TDataCold* m_data_cold = nullptr;

        uint16_t* m_free_list = nullptr;
        uint16_t* m_generations = nullptr;
        size_t     m_free_list_head = 0;
    };
}