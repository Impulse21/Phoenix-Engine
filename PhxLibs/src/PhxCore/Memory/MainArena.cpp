#include <PhxCore/PhxCore_pch.h>
#include "MemoryArenaInterfaces.h"

#include <PhxCore/Platform/PlatformWrapper.h>

#include <tlsf.h>

using namespace phx;

namespace
{
	void ExitWalker(void* ptr, size_t size, int used, void* user)
	{
		MemoryStatistics* stats = (MemoryStatistics*)user;
		stats->Add(used ? size : 0);

		if (used)
			PHX_CORE_WARN("Found active allocation {0}, {1}", ptr, size);
	}
}

void MainArena::Initialize(size_t reserveBytes, size_t initialCommitBytes)
{
	m_baseAddress =  Platform::Get().VirtualMemReserve(reserveBytes);
	m_reservedSize = reserveBytes;

	m_pageSize = initialCommitBytes;
	
	uint8_t* ptr = static_cast<uint8_t*>(m_baseAddress) + m_commitedSize;
	Platform::Get().VirtualMemCommit(ptr, m_pageSize);
	m_commitedSize = initialCommitBytes;

	m_tlsfHandle = tlsf_create_with_pool(m_baseAddress, m_commitedSize);

}

void MainArena::Shutdown()
{
	MemoryStatistics stats{ 0, m_commitedSize };
	pool_t pool = tlsf_get_pool(m_tlsfHandle);

	tlsf_walk_pool(pool, ExitWalker, (void*)&stats);

	// Free the committed memory
	if (!Platform::Get().VirtualMemFree(m_baseAddress))
	{
		PHX_CORE_ERROR("Failed to free virtual memory");
	}

	// Free the committed memory
	if (!Platform::Get().VirtualMemFree(m_baseAddress))
	{
		PHX_CORE_ERROR("Failed to free virtual memory");
	}

	m_baseAddress = nullptr;
	m_pageSize = 0;
	m_reservedSize = 0;
	m_commitedSize = 0;
}

void* MainArena::Allocate(size_t size, size_t alignment)
{
#if defined (HEAP_ALLOCATOR_STATS)
	void* allocatedMemory = alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
	sizet actualSize = tlsf_block_size(allocatedMemory);
	m_allocatedSize += actualSize;

	/*if ( size == 52224 ) {
		return allocatedMemory;
	}*/
#else
	void* allocatedMemory = alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
#endif // HEAP_ALLOCATOR_STATS

	if (allocatedMemory)
		return allocatedMemory;


	uint8_t* ptr = static_cast<uint8_t*>(m_baseAddress) + m_commitedSize;
	Platform::Get().VirtualMemCommit(ptr, m_pageSize);
	m_commitedSize += size;

	tlsf_add_pool(m_tlsfHandle, ptr, m_pageSize);


#if defined (HEAP_ALLOCATOR_STATS)
	allocatedMemory = alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
	sizet actualSize = tlsf_block_size(allocatedMemory);
	m_allocatedSize += actualSize;

	/*if ( size == 52224 ) {
		return allocatedMemory;
	}*/
#else
	allocatedMemory = alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
#endif // HEAP_ALLOCATOR_STATS

	return allocatedMemory;
}

void MainArena::Deallocate(void* pointer)
{
#if defined (HEAP_ALLOCATOR_STATS)
	sizet actualSize = tlsf_block_size(pointer);
	m_allocatedSize -= actualSize;

	tlsf_free(m_tlsfHandle, pointer);
#else
	tlsf_free(m_tlsfHandle, pointer);
#endif
}

bool MainArena::IsAddressInRange(const void* ptr) const
{ 
	if (!m_baseAddress || m_commitedSize== 0 || !ptr)
		return false;

	const char* p_char = static_cast<const char*>(ptr);
	const char* base_char = static_cast<const char*>(m_baseAddress);

	return (p_char >= base_char) && (p_char < (base_char + m_commitedSize));
}
