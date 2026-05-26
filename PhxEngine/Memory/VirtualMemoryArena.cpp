#include "VirtualMemoryArena.h"

#include <PhxEngine/Platform/VirtualMemory.h>

using namespace phx;

void phx::VirtualMemoryArena::Initialize(const Descriptor& desc)
{
    m_page_size = platform::VirtualMemory::GetAllocationGranularity();

    m_base = platform::VirtualMemory::Reserve(desc.reserved_size);
    if (m_base == nullptr)
        return;

    m_reserved = desc.reserved_size;
}

void phx::VirtualMemoryArena::Shutdown()
{
  if (!m_base) 
    return;

  platform::VirtualMemory::Release(m_base, m_reserved);

  m_base = nullptr;
  m_reserved = 0;
  m_committed = 0;
  m_offset = 0;
}

void* phx::VirtualMemoryArena::Alloc(usize size)
{
    void* ptr = Carve(size);
    if (!Commit(ptr, size))
        return nullptr;
    return ptr;
}

void* phx::VirtualMemoryArena::Carve(usize reserve_size)
{
    const usize aligned = (reserve_size + m_page_size - 1) & ~(m_page_size - 1);

    PHX_ASSERT((m_offset + aligned) <= m_reserved);
    void* ptr = static_cast<char*>(m_base) + m_offset;
    m_offset += aligned;
    return ptr;
}

bool phx::VirtualMemoryArena::Commit(void* ptr, usize size)
{
    const usize aligned = (size + m_page_size - 1) & ~(m_page_size - 1);

    if (!platform::VirtualMemory::Commit(ptr, aligned))
        return false;

    m_committed += aligned;
    return true;
}