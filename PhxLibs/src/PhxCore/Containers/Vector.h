#pragma once

#include <PhxCore/Memory.h>
#include "EastAllocator.h"
#include <EASTL/vector.h>

namespace phx
{

    template<typename T>
    using Vector = eastl::vector<T, EastlAllocator<HeapAllocator>>;
}