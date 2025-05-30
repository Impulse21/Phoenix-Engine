#pragma once


#include <PhxCore/Platform/BasePlatformWrapper.h>

namespace phx::platform::windows
{
    class WindowsPlatformWrapperImpl : public BasePlatformWrapper<WindowsPlatformWrapperImpl>
    {
        friend class BasePlatformWrapper<WindowsPlatformWrapperImpl>;
    public:
        WindowsPlatformWrapperImpl() = default;
        ~WindowsPlatformWrapperImpl() = default;

        void* PlatformVirtualMemReserve(size_t reserveSize);
        void PlatformVirtualMemCommit(void* ptr, size_t commitSize);
        bool PlatformVirtualMemFree(void* ptr);
        
    };
}