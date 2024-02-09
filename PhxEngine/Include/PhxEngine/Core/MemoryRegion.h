#pragma once

#include <memory>

//
// This corresponds to a marc::Region that owns the memory it is loaded to.
//
template<typename T>
class MemoryRegion
{
public:
    MemoryRegion() = default;

    MemoryRegion(std::unique_ptr<char[]> buffer)
        : m_buffer(std::move(buffer))
    {
    }

    char* Data()
    {
        return this->m_buffer.get();
    }

    T* Get()
    {
        return reinterpret_cast<T*>(this->m_buffer.get());
    }

    T* operator->()
    {
        return reinterpret_cast<T*>(this->m_buffer.get());
    }

    T const* operator->() const
    {
        return reinterpret_cast<T const*>(this->m_buffer.get());
    }

private:
    std::unique_ptr<char[]> m_buffer;
};
