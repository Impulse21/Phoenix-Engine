#pragma once

#include <PhxCore/Platform/PlatformTypes.h>
#include <PhxCore/Platform/PlatformConfig.h>

#include <PhxCore/Span.h>
#include <PhxCore/Result.h>
#include "PhxCore/Memory/MemoryUtils.h"

namespace phx::Platform
{
    // -- Virtual memory ---
    void *VirtualMemReserve(size_t reserveSize);
    void VirtualMemCommit(void *ptr, size_t commitSize);
    bool VirtualMemFree(void *ptr);

    template <typename T, size_t _PageSize = 1>
    T *VirtualMemReserveTyped(size_t numEntries)
    {
        void *alloc = VirtualMemReserve(AlignUp(numEntries * sizeof(T), _PageSize));
        return static_cast<T *>(alloc);
    }
    
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
    phx::Result<PlatformFileHandle> OpenFile(const std::string &os_path, FileMode mode)
    {
        const char* mode_str = GetModeString(mode);
        return OpenFile(os_path, mode_str);
    }

    // -- Resources ---
    phx::Result<phx::Span<char>> GetEmbeddedResource(std::string const &resource_name);

    // -- Thread stuff ---
    enum class ThreadPriority
    {
        High = 0,
        Normal,
        Low,
    };


    void SetThreadName(std::thread& thread, const std::string& name);
    void SetThreadAffinity(std::thread& thread, int affinity);
    void SetThreadPriority(std::thread& thread, ThreadPriority prio);
}