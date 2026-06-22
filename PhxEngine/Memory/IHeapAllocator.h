#pragma once

#include "IAllocator.h"

namespace phx
{
    class IHeapAllocator : public IAllocator
    {
    public:
        virtual ~IHeapAllocator() = default;
    };
}