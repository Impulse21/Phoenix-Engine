#pragma once

#include <PhxCore/Memory.h>

namespace phx::platform
{
    template<class TDerived>
    class BasePlatformWrapper
    {
    public:

        void* VirtualMemReserve(size_t reserveSize)
        {
            return static_cast<TDerived*>(this)->PlatformVirtualMemReserve(reserveSize);
        }

        template<typename T, size_t _PageSize = 1>
        T* VirtualMemReserveTyped(size_t numEntries)
        {
            void* alloc = VirtualMemReserve(AlignUp(numEntries * sizeof(T), _PageSize));
            return static_cast<T*>(alloc);
        }

        void VirtualMemCommit(void* ptr, size_t commitSize)
        {
            return static_cast<TDerived*>(this)->PlatformVirtualMemCommit(ptr, commitSize);
        }

        bool VirtualMemFree(void* ptr)
        {
            return static_cast<TDerived*>(this)->PlatformVirtualMemFree(ptr);
        }

    protected:
        BasePlatformWrapper() = default;
        ~BasePlatformWrapper() = default;
    };

}
