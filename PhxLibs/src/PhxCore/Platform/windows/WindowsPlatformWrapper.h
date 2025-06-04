#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <SDKDDKVer.h>
#include <windows.h>
#include <tchar.h>


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
        
        phx::Result<PlatformFileAttributes>  PlatformGetFileAttributes(std::string const& path);
    };
}