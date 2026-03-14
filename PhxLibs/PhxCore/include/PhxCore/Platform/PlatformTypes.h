#pragma once

#include <chrono>
#include <PhxCore/EnumUtils.h>

namespace phx
{
    // --- Platform Agnostic File Attributes (from previous definition) ---
    // TODO - Consider moving this to IO?. Also drop the HI
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

    enum class FileMode 
    {
        Read    = BIT(0),  // 'r'
        Write   = BIT(1),  // 'w'
        Append  = BIT(2),  // 'a'
        Update  = BIT(3),  // '+'
        Binary  = BIT(4)   // 'b'
    };
    PHX_ENUM_CLASS_FLAGS(FileMode);

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

}