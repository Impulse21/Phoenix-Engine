#pragma once


// This is disabled for now. At the time I was trying to be cute
// had control heap allocations. This is causing to much upfront costs.
// I will stick with system memory for core allocations.
#if false
#include "IAllocator.h"

namespace phx
{
    class IHeapAllocator : public IAllocator
    {
    public:
        virtual ~IHeapAllocator() = default;
    };
}
#endif