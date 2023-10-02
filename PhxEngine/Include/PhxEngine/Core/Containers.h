#pragma once

#include <vector>
#include <PhxEngine/Core/Memory.h>

namespace PhxEngine::Core
{
    template <typename T, size_t CapacityStep = 16>
    class FlexArray {
    public:
        FlexArray() 
            : m_data(nullptr)
            , m_size(0)
            , m_capacity(0) 
        {
        }

        ~FlexArray() 
        {
            if (this->m_data)
            {
                phx_delete_arr(this->m_data);
            }
        }

        void push_back(const T& value) 
        {
            if (this->m_size == this->m_capacity) 
            {
                // If the array is full, double its capacity
                this->m_capacity = (this->m_capacity == 0) ? 1 : this->m_capacity * CapacityStep;
                if (this->m_data)
                {
                    this->m_data = phx_realloc_arr(T, this->m_data, this->m_capacity);
                }
                else
                {
                    this->m_data = phx_new_arr(T, this->m_capacity);
                }
            }

            this->m_data[this->m_size++] = value;
        }

        T& operator[](size_t index) 
        {
            if (index >= this->m_size)
            {
                throw std::out_of_range("Index out of range");
            }

            return this->m_data[index];
        }

        void clear()
        {
            for (size_t i = 0; i < this->m_size; i++)
            {
                if (!std::is_trivially_destructible<T>::value)
                {
                    this->m_data[i]->~T();
                }
            }
            std::memset(this->m_data, 0, sizeof(T) * this->m_size);
            this->m_size = 0;
        }

        size_t size() const 
        {
            return this->m_size;
        }

        const T* data() const
        {
            return this->m_data;
        }

        size_t capacity() const 
        {
            return this->m_capacity;
        }

    private:
        T* m_data;
        size_t m_size;
        size_t m_capacity;
    };
}