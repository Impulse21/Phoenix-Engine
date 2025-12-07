#pragma once

#include "PlatformConfig.h"
#include "PhxCore/Memory/MemoryUtils.h"

namespace phx::Platform
{
    inline platform::PlatformWrapper& Get()
    {
        static platform::PlatformWrapper s_instance;
        return s_instance;
    }

    template<typename T, size_t _PageSize = 1>
    T* VirtualMemReserve(platform::PlatformWrapper& platform, size_t numEntries)
    {
        void* alloc = platform.VirtualMemReserve(phx::AlignUp(numEntries * sizeof(T), _PageSize));
        return static_cast<T*>(alloc);
    }
}