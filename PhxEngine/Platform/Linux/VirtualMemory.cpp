#include <PhxEngine/Platform/VirtualMemory.h>


#include <sys/mman.h>    // mmap, munmap, mprotect      
#include <unistd.h>


namespace phx::platform::VirtualMemory
{
    usize GetPageSize()
    {
        return static_cast<usize>(sysconf(_SC_PAGESIZE));
    }

    usize GetAllocationGranularity()
    {
        return GetPageSize();  // On Linux, allocation granularity is the same as page size
    }

    void* Reserve(usize size)
    {

        void* ptr = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptr == MAP_FAILED) 
        {
            return nullptr;
        }

        return ptr;
    }

    bool Commit(void* ptr, usize size)
    {
        return mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
    }

    void Release(void* ptr, usize size)
    {
        munmap(ptr, size);
    }
}