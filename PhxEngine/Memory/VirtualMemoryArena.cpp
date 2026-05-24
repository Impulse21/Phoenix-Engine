#include "VirtualMemoryArena.h"

#include <PhxEngine/Platform/VirtualMemory.h>

using namespace phx;

void phx::VirtualMemoryArena::Initialize(const Descriptor& desc)
{
    m_page_size = platform::VirtualMemory::GetAllocationGranularity();
}

bool phx::VirtualMemoryArena::Commit(void* ptr, usize size)
{
    usize aligned = (size + m_page_size - 1) & ~(m_page_size - 1);

    if (platform::VirtualMemory::Commit(ptr, aligned))
    {
        m_committed += aligned;
        return true;
    }

    return false;
}