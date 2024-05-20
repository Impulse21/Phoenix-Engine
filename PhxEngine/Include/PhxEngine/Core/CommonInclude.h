#pragma once
#include <cstdint>

namespace phx
{
	// type-alias
	using uint8 = uint8_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;
    using size = size_t;

    struct NonMoveable
    {
        NonMoveable() = default;
        NonMoveable(NonMoveable&&) = delete;
        NonMoveable& operator = (NonMoveable&&) = delete;
    };

    struct NonCopyable
    {
        NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
}