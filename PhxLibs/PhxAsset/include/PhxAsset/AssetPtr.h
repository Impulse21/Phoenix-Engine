#pragma once

namespace phx::asset
{
    template<typename T>
    struct Ptr
    {
        T* ptr;

        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }

        explicit operator bool() const
        {
            return ptr != nullptr;
        }

        Ptr(T* ptr)
            : ptr(ptr)
        {

        }
        
        Ptr()
            : ptr(nullptr){};
    };
}