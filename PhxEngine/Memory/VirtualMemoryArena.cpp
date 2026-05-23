#include "VirtualMemoryArena.h"

using namespace phx;

bool phx::VirtualMemoryArena::Init(const Desc& desc)
{
#if defined(PHX_PLATFORM_WINDOWS)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    m_pageSize = info.dwAllocationGranularity;  // 64KB on Windows
                                                // NOT dwPageSize (4KB)
                                                // VirtualAlloc must align to this
#elif defined(PHX_PLATFORM_LINUX)
    m_pageSize = (usize)sysconf(_SC_PAGESIZE);  // typically 4KB
#endif

    // ...rest of init
}

bool phx::VirtualMemoryArena::Commit(void* ptr, usize size)
{
    // Always round up to page granularity before committing
    usize aligned = (size + m_pageSize - 1) & ~(m_pageSize - 1);

#if defined(PHX_PLATFORM_WINDOWS)
    void* result = VirtualAlloc(ptr, aligned, MEM_COMMIT, PAGE_READWRITE);
    if (!result) return false;
#elif defined(PHX_PLATFORM_LINUX)
    if (mprotect(ptr, aligned, PROT_READ | PROT_WRITE) != 0) return false;
#endif

    m_committed += aligned;
    return true;
}