#include <PhxEngine/Platform/VirtualMemory.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace phx::platform::VirtualMemory
{
    usize GetPageSize()
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        return sys_info.dwPageSize;
    }

    usize GetAllocationGranularity()
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        return sys_info.dwAllocationGranularity;
    }

    void* Reserve(usize size)
    {
        return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
    }

    bool Commit(void* ptr, usize size)
    {
        return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
    }

    void Release(void* ptr, usize size)
    {
        VirtualFree(ptr, size, MEM_RELEASE);
    }
}