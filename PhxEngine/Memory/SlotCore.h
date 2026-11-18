#pragma once

#include <PhxEngine/Core/PhxDefines.h>

#include <algorithm>
#include <vector>

namespace phx
{
    // Fixed-capacity index allocation over [0, max_slots): hands out never-used
    // indices in order, then recycles freed ones (LIFO).
    //
    // Knows nothing about what the indices address (descriptor heap slots,
    // pool entries, ...). Not thread-safe: callers serialize Allocate/Free.
    template<typename T = u32, T invalid_value = T(-1)>
    class SlotCore
    {
    public:
        static constexpr T kInvalid = invalid_value;

    public:
        void Initialize(T max_slots)
        {
            m_max_slots      = max_slots;
            m_next_new_index = 0;
            m_free_list.clear();
        }

        // Returns kInvalid when every slot is in use.
        [[nodiscard]] T Allocate()
        {
            if (!m_free_list.empty())
            {
                const T idx = m_free_list.back();
                m_free_list.pop_back();
                return idx;
            }

            if (m_next_new_index < m_max_slots)
                return m_next_new_index++;

            return invalid_value;
        }

        void Free(T index)
        {
            PHX_ASSERT(index != invalid_value);
            PHX_ASSERT(index < m_next_new_index && "SlotCore::Free: index was never allocated");
#if defined(PHX_DEBUG)
            // O(n) double-free check; debug-only.
            PHX_ASSERT(std::find(m_free_list.begin(), m_free_list.end(), index) == m_free_list.end()
                && "SlotCore::Free: slot freed twice");
#endif
            m_free_list.push_back(index);
        }

        [[nodiscard]] T Capacity()  const { return m_max_slots; }
        [[nodiscard]] T UsedCount() const { return m_next_new_index - static_cast<T>(m_free_list.size()); }

    private:
        T m_max_slots      = 0;
        T m_next_new_index = 0;
        std::vector<T> m_free_list;
    };
}
