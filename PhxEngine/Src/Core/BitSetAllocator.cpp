#include "PhxEngine\Core\BitSetAllocator.h"

PhxEngine::Core::BitSetAllocator::BitSetAllocator(size_t capacity)
    : m_nextAvailable(0)
{
    this->m_allocated.resize(capacity);
}

int PhxEngine::Core::BitSetAllocator::Allocate()
{
    std::scoped_lock _(this->m_mutex);
    
    int result = -1;

    int capacity = static_cast<int>(this->m_allocated.size());
    for (int i = 0; i < capacity; i++)
    {
        int ii = (this->m_nextAvailable + i) % capacity;

        if (!this->m_allocated[ii])
        {
            result = ii;
            this->m_nextAvailable = (ii + 1) % capacity;
            this->m_allocated[ii] = true;
            break;
        }
    }

    return result;
}

void PhxEngine::Core::BitSetAllocator::Release(int index)
{
    if (index >= 0 && index < static_cast<int>(this->m_allocated.size()))
    {
        std::scoped_lock _(this->m_mutex);

        this->m_allocated[index] = false;
        this->m_nextAvailable = std::min(this->m_nextAvailable, index);
    }
}
