#pragma once

#include <functional>

namespace phx
{
    inline void HashCombine(std::size_t& seed, std::size_t value)
    {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <typename T>
    inline void HashCombine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        HashCombine(seed, hasher(v));
    }
}