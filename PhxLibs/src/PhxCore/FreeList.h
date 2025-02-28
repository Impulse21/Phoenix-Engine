#pragma once

#include <atomic>
#include <array>
#include <cstddef>

namespace phx
{
    template <std::size_t Capacity>
    class FreeList 
    {
    public:
        FreeList() 
        {
            for (std::size_t i = 0; i < Capacity; ++i) 
            {
                m_nodes[i].Next.store(i + 1, std::memory_order_relaxed);
            }

            m_nodes[Capacity - 1].Next.store(-1, std::memory_order_relaxed); // End of list
            m_head.store(0, std::memory_order_relaxed);
        }

        bool allocate(std::size_t& index) 
        {
            std::size_t currentHead = m_head.load(std::memory_order_acquire);
            while (currentHead != -1) 
            {
                std::size_t next = m_nodes[currentHead].Next.load(std::memory_order_acquire);
                if (m_head.compare_exchange_weak(currentHead, next, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    index = currentHead;
                    return true;
                }
            }
            return false; // No available indices
        }

        void free(std::size_t index) 
        {
            if (index >= Capacity) 
            {
                return; // Ignore invalid indices
            }

            std::size_t currentHead = m_head.load(std::memory_order_acquire);
            do 
            {
                m_nodes[index].Next.store(currentHead, std::memory_order_release);
            } 
            while (!m_head.compare_exchange_weak(currentHead, index, std::memory_order_acq_rel, std::memory_order_acquire));
        }

    private:
        struct Node 
        {
            std::atomic<std::size_t> Next;
        };

        std::array<Node, Capacity> m_nodes;
        std::atomic<std::size_t> m_head;
    };

}