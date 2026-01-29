#pragma once

#include <PhxCore/Result.h>
#include <PhxCore/Span.h>

#include <PhxCore/EnumUtils.h>
#include <chrono>

namespace phx::platform
{
    // --- Platform Agnostic File Attributes (from previous definition) ---
    enum class PlatformFileType 
    {
        File,
        Directory,
        DoesNotExist,
        OtherOrError
    };

    using Timestamp = std::chrono::system_clock::time_point;

    enum class FileSeekOrigin 
    {
        Begin,
        Current,
        End
    };

    struct PlatformFileAttributes 
    {
        PlatformFileType type = PlatformFileType::OtherOrError;
        uint64_t size = 0;
        Timestamp creation_time;
        Timestamp last_access_time;
        Timestamp last_write_time;
        bool is_read_only = false;
        bool is_hidden = false;

        PlatformFileAttributes()
            : creation_time(Timestamp{}),
            last_access_time(Timestamp{}),
            last_write_time(Timestamp{}) 
        {
        }
    };

    struct PlatformFileHandle
    {
        void* internal_handle = nullptr;

        bool IsValid() const { return internal_handle != nullptr; }
        bool operator==(const PlatformFileHandle& other) const { return internal_handle == other.internal_handle; }

        template<typename T>
        T* As() { return static_cast<T*>(internal_handle); }
    };

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

        phx::Result<PlatformFileAttributes> GetFileAttr(std::string const& path)
        {
            return static_cast<TDerived*>(this)->PlatformGetFileAttributes(path);
        }

        phx::Result<std::string> GetExectuablePath()
        {
            return static_cast<TDerived*>(this)->PlatformGetExectuablePath();
        }

        phx::Result<PlatformFileHandle> OpenFile(const std::string& os_path, const char* mode)
        {
            return static_cast<TDerived*>(this)->PlatformOpenFile(os_path, mode);
        }

        phx::Result<phx::Span<char>> GetEmbeddedResource(std::string const& resource_name)
        {
            return static_cast<TDerived*>(this)->PlatformGetEmbeddedResource(resource_name);
        }

        void CloseFile(PlatformFileHandle handle)
        {
            static_cast<TDerived*>(this)->PlatformCloseFile(handle);
        }

        bool SeekFile(PlatformFileHandle handle, int64_t offset, FileSeekOrigin origin)
        {
            return static_cast<TDerived*>(this)->PlatformSeekFile(handle, offset, origin);
        }

        size_t ReadFile(PlatformFileHandle handle, void* buffer, size_t size_to_read)
        {
            return static_cast<TDerived*>(this)->PlatformReadFile(handle, buffer, size_to_read);
        }

        void WriteFile(PlatformFileHandle handle, const char* buffer, size_t size_to_write)
        {
            return static_cast<TDerived*>(this)->PlatformWriteFile(handle, buffer, size_to_write);
        }

    protected:
        BasePlatformWrapper() = default;
        ~BasePlatformWrapper() = default;
    };
}
