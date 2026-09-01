#pragma once

#include <PhxEngine/Core/Result.h>
#include <PhxEngine/Core/EnumUtils.h>

#include <chrono>
#include <string>

namespace phx::platform
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
        Read    = PHX_BIT(0),  // 'r'
        Write   = PHX_BIT(1),  // 'w'
        Append  = PHX_BIT(2),  // 'a'
        Update  = PHX_BIT(3),  // '+'
        Binary  = PHX_BIT(4)   // 'b'
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


    // -- File System ---
    phx::Result<std::string> GetExectuablePath();
    phx::Result<PlatformFileAttributes> GetFileAttr(std::string const &path);
    void CloseFile(PlatformFileHandle handle);
    bool SeekFile(PlatformFileHandle handle, int64_t offset, FileSeekOrigin origin);
    size_t ReadFile(PlatformFileHandle handle, void *buffer, size_t size_to_read);
    void WriteFile(PlatformFileHandle handle, const char *buffer, size_t size_to_write);

    // -- File IO ---
    inline const char* GetModeString(FileMode mode) 
    {
           int m = static_cast<int>(mode);

           // Append Logic ('a')
           if (m & (int)FileMode::Append) 
           {
               if (m & (int)FileMode::Update) 
                   return (m & (int)FileMode::Binary) ? "a+b" : "a+";

               return (m & (int)FileMode::Binary) ? "ab" : "a";
           }

           // Write Logic ('w')
           if (m & (int)FileMode::Write) 
           {
               if (m & (int)FileMode::Update) 
                   return (m & (int)FileMode::Binary) ? "w+b" : "w+";

               return (m & (int)FileMode::Binary) ? "wb" : "w";
           }

           // Read Logic ('r') - Default
           if (m & (int)FileMode::Update) 
               return (m & (int)FileMode::Binary) ? "r+b" : "r+";
               
           return (m & (int)FileMode::Binary) ? "rb" : "r";
    }


    phx::Result<PlatformFileHandle> OpenFile(const std::string &os_path, const char *mode);
    inline phx::Result<PlatformFileHandle> OpenFile(const std::string &os_path, FileMode mode)
    {
        const char* mode_str = GetModeString(mode);
        return OpenFile(os_path, mode_str);
    }

}