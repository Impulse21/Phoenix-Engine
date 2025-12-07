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
        phx::Result<std::string> PlatformGetExectuablePath();
        
        phx::Result<PlatformFileAttributes>  PlatformGetFileAttributes(std::string const& path);

        phx::Result<PlatformFileHandle> PlatformOpenFile(const std::string& os_path, const char* mode);
        void PlatformCloseFile(PlatformFileHandle handle);

        bool PlatformSeekFile(PlatformFileHandle handle, int64_t offset, FileSeekOrigin origin);
        size_t PlatformReadFile(PlatformFileHandle handle, void* buffer, size_t size_to_read);

        void PlatformWriteFile(PlatformFileHandle handle, const char* buffer, size_t size_to_write);

        phx::Result<phx::Span<char>> PlatformGetEmbeddedResource(std::string const& resource_name);
    };
}