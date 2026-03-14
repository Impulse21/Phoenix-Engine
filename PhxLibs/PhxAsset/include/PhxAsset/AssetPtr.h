#pragma once

namespace phx
{
    template<typename T>
    struct AssetPtr
    {
        T* ptr;

        T* operator->() const { return ptr; }
        T& operator*() const { return *ptr; }

        explicit operator bool() const
        {
            return ptr != nullptr;
        }
    };
}