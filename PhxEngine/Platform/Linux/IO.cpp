#include <PhxEngine/Platform/IO.h>

#include <PhxEngine/Core/Log.h>

#include <sys/mman.h>    // mmap, munmap, mprotect
#include <sys/stat.h>    // stat
#include <unistd.h>      // readlink
#include <limits.h>      // PATH_MAX.
#include <cerrno>
#include <cstdio>
#include <cstring>


#include <sys/resource.h>  // setpriority, PRIO_PROCESS
#include <sys/syscall.h>   // SYS_gettid
#include <unistd.h>        // syscall
#include <pthread.h>       // pthread_t, pthread_setschedparam
#include <sched.h>         // SCHED_RR, sched_get_priority_max

using namespace phx;

phx::Result<std::string> platform::GetExectuablePath()
{
    char path[PATH_MAX] = { 0 };
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1)
		return Unexpected(ResultError::Failure);

    path[len] = '\0';
    return std::string(path);
}

phx::Result<platform::PlatformFileAttributes> platform::GetFileAttr(std::string const& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        PHX_LOG_ERROR(Log::Channels::Platform, "Failed to retrieve platform file attributes: {0}", path);
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

phx::Result<platform::PlatformFileHandle> platform::OpenFile(const std::string& os_path, const char* mode)
{
    FILE* fp = fopen(os_path.c_str(), mode);
    if (!fp)
        return Unexpected(ResultError::Failure);

    return platform::PlatformFileHandle{ fp };
}

void platform::CloseFile(platform::PlatformFileHandle handle)
{
    if (handle.IsValid())
        fclose(handle.As<FILE>());
}

bool platform::SeekFile(platform::PlatformFileHandle handle, int64_t offset, platform::FileSeekOrigin origin)
{
    if (!handle.IsValid())
        return false;

    int whence = 0;
    switch (origin)
    {
    case platform::FileSeekOrigin::Begin:   whence = SEEK_SET; break;
    case platform::FileSeekOrigin::Current: whence = SEEK_CUR; break;
    case platform::FileSeekOrigin::End:     whence = SEEK_END; break;
    }

    return fseeko(handle.As<FILE>(), static_cast<off_t>(offset), whence) == 0;
}

void platform::WriteFile(PlatformFileHandle handle, const char* buffer, size_t size_to_write)
{
    if (!handle.IsValid() || !buffer || size_to_write == 0)
        return;

    fwrite(buffer, 1, size_to_write, handle.As<FILE>());
}

size_t platform::ReadFile(PlatformFileHandle handle, void* buffer, size_t size_to_read)
{
    if (!handle.IsValid() || !buffer || size_to_read == 0)
        return 0;

    return fread(buffer, 1, size_to_read, handle.As<FILE>());
}
