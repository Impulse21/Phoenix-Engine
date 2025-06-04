#pragma once

#include <PhxCore/Result.h>
#include <PhxCore/Memory/MemorySystem.h>
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
    protected:
        BasePlatformWrapper() = default;
        ~BasePlatformWrapper() = default;
    };

}
