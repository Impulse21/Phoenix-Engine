#pragma once

namespace phx
{
    class IAllocator
    {
    public:
        virtual ~IAllocator() = default;

        [[nodiscard]] virtual void* Alloc(usize size, usize alignment = 16) = 0;
        virtual void Free(void* ptr) = 0;
    };
}