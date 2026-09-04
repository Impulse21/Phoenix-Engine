#pragma once

#include <vector>
namespace phx
{
    template<typename T = u32, T invalid_value = T(-1)>
    class SlotAllocator
    {
    public:
        void Initialize(uint32_t max_slots)
        {
            m_max_slots = max_slots;
            m_next_new_index = 0;
        }

        T AllocateSlot()
        {
            if (!m_free_list.empty()) 
            {
                auto idx = m_free_list.back();
                m_free_list.pop_back();

                return idx;
            }

            if (m_next_new_index < m_max_slots) 
            {
                return m_next_new_index++;
            }

            return invalid_value;
        }

        void FreeSlot(T index)
        {
            m_free_list.push_back(index);
        }

    private:
        T m_max_slots;
        T  m_next_new_index;
        std::vector<T> m_free_list;
    };
   
}