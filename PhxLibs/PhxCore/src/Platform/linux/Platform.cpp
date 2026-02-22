#include "PhxCore_pch.h"

#ifdef PHX_PLATFORM_LINUX
#include <PhxCore/Platform/Platform.h>
#include <PhxCore/StringUtils.h>

#include <unordered_map>

// POSIX / Linux headers
#include <sys/mman.h>    // mmap, munmap, mprotect
#include <sys/stat.h>    // stat
#include <unistd.h>      // readlink
#include <limits.h>      // PATH_MAX.
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <pthread.h>
using namespace phx;

// ----------------------------------------------------------------------------
// Virtual Memory
// ----------------------------------------------------------------------------

namespace
{
    std::unordered_map<void*, size_t> g_reserved_blocks; // Track reserved blocks and their sizes for munmap
}

void* Platform::VirtualMemReserve(size_t reserveSize)
{
    // Reserve without backing - PROT_NONE means no access until committed
    void* ptr = mmap(nullptr, reserveSize, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == MAP_FAILED)
    {
        PHX_CORE_ERROR("VirtualMemReserve failed: {0}", strerror(errno));
        return nullptr;
    }

    g_reserved_blocks[ptr] = reserveSize; // Track the reservation size

    return ptr;
}

void Platform::VirtualMemCommit(void* ptr, size_t commitSize)
{
    mprotect(ptr, commitSize, PROT_READ | PROT_WRITE);
}

bool Platform::VirtualMemFree(void* ptr)
{
    if (ptr == nullptr)
        return false;

    auto it = g_reserved_blocks.find(ptr);
    if (it == g_reserved_blocks.end())
    {
        PHX_CORE_ERROR("Attempted to free unrecognized memory block");
        return false;
    }

    const size_t size = it->second;
    return munmap(ptr, size) == 0;
}

// ----------------------------------------------------------------------------
// File System
// ----------------------------------------------------------------------------

phx::Result<std::string> Platform::GetExectuablePath()
{
    char path[PATH_MAX] = { 0 };
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
		return Unexpected(ResultError::Failure);

    path[len] = '\0';
    return std::string(path);
}

phx::Result<PlatformFileAttributes> Platform::GetFileAttr(std::string const& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        PHX_CORE_WARN("Failed to retrieve platform file attributes: {0}", path);
        return Unexpected(ResultError::Failure);
    }

    PlatformFileAttributes attrs;
    attrs.type = S_ISDIR(st.st_mode)
        ? PlatformFileType::Directory
        : PlatformFileType::File;

    attrs.size = static_cast<uint64_t>(st.st_size);
    attrs.is_read_only = !(st.st_mode & S_IWUSR);
    attrs.is_hidden = !path.empty() && path[path.find_last_of('/') + 1] == '.';

    // Convert timespec to system_clock::time_point
    auto to_tp = [](const struct timespec& ts) -> Timestamp {
        auto dur = std::chrono::seconds(ts.tv_sec)
                 + std::chrono::nanoseconds(ts.tv_nsec);
        return std::chrono::system_clock::time_point(
            std::chrono::duration_cast<std::chrono::system_clock::duration>(dur));
    };

    attrs.creation_time    = to_tp(st.st_ctim); // ctime = metadata change, not creation
    attrs.last_access_time = to_tp(st.st_atim);
    attrs.last_write_time  = to_tp(st.st_mtim);

    return attrs;
}

phx::Result<PlatformFileHandle> Platform::OpenFile(const std::string& os_path, const char* mode)
{
    FILE* fp = fopen(os_path.c_str(), mode);
    if (!fp)
        return Unexpected(ResultError::Failure);

    return PlatformFileHandle{ fp };
}

void Platform::CloseFile(PlatformFileHandle handle)
{
    if (handle.IsValid())
        fclose(handle.As<FILE>());
}

bool Platform::SeekFile(PlatformFileHandle handle, int64_t offset, FileSeekOrigin origin)
{
    if (!handle.IsValid())
        return false;

    int whence = 0;
    switch (origin)
    {
    case FileSeekOrigin::Begin:   whence = SEEK_SET; break;
    case FileSeekOrigin::Current: whence = SEEK_CUR; break;
    case FileSeekOrigin::End:     whence = SEEK_END; break;
    }

    return fseeko(handle.As<FILE>(), static_cast<off_t>(offset), whence) == 0;
}

void Platform::WriteFile(PlatformFileHandle handle, const char* buffer, size_t size_to_write)
{
    if (!handle.IsValid() || !buffer || size_to_write == 0)
        return;

    fwrite(buffer, 1, size_to_write, handle.As<FILE>());
}

size_t Platform::ReadFile(PlatformFileHandle handle, void* buffer, size_t size_to_read)
{
    if (!handle.IsValid() || !buffer || size_to_read == 0)
        return 0;

    return fread(buffer, 1, size_to_read, handle.As<FILE>());
}

// ----------------------------------------------------------------------------
// Embedded Resources
// ----------------------------------------------------------------------------

phx::Result<phx::Span<char>> Platform::GetEmbeddedResource(std::string const& resource_name)
{
    // Linux has no direct equivalent of Win32 RT_RCDATA resources.
    // The idiomatic approach is to link binary blobs as object files using
    // objcopy, then reference the generated symbols. See note below.

    // Pattern: extern char _binary_<name>_start[], _binary_<name>_end[];
    // These are generated by: objcopy --input binary --output elf64-x86-64 \
    //     --binary-architecture i386 resource.bin resource.o

    // Since resource names are runtime strings, you'd maintain a registry.
    // Placeholder - wire up your own resource registry here.
    (void)resource_name;
    PHX_CORE_ASSERT(false, "GetEmbeddedResource not implemented - TODO: set up your own resource registry");
    return Unexpected(ResultError::Failure);
}

void phx::Platform::SetThreadName(std::thread& thread, const std::string& name)
{
    pthread_t handle = thread.native_handle();

    int nameResult = pthread_setname_np(handle, name.substr(0, 15).c_str());
    PHX_ASSERT(nameResult == 0);
}

void phx::Platform::SetThreadAffinity(std::thread& thread, int affinity)
{
    pthread_t handle = thread.native_handle();

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(affinity, &cpu_set);

    int affinityResult = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpu_set);
    PHX_ASSERT(affinityResult == 0); // In POSIX, 0 indicates success
}

void phx::Platform::SetThreadPriority(std::thread& thread, int prio)
{
    pthread_t handle = thread.native_handle();

    struct sched_param param;
    param.sched_priority = prio;
    PHX_ASSERT(pthread_setschedparam(handle, SCHED_OTHER, &param) == 0);
}
#endif