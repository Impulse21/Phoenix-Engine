#include <PhxCore/PhxCore_pch.h>
#include "MemoryArenaInterfaces.h"


void* phx::MallocAllocator::Allocate(size_t size, size_t)
{
	return malloc(size);
}

void* phx::MallocAllocator::Allocate(size_t size, size_t, const char*, int32_t)
{
	return malloc(size);
}

void phx::MallocAllocator::Deallocate(void* pointer)
{
	free(pointer);
}

#if false

void PagedLinearAllocator::Initialize(size_t size)
{
	m_memory = static_cast<uint8_t*>(
		Platform::Get().VirtualMemReserve(size));
	m_commitedSize = 0;
	m_reservedSize = size;

}

void PagedLinearAllocator::Shutdown()
{
	// Free the committed memory
	if (!Platform::Get().VirtualMemFree(m_memory))
	{
		PHX_CORE_ERROR("Failed to free virtual memory");
	}

	m_memory = nullptr;
	m_reservedSize = 0;
	m_commitedSize = 0;
}

void* PagedLinearAllocator::Allocate(size_t size, size_t alignment)
{
	const size_t newStart = AlignUp(m_commitedSize, alignment);
	PHX_CORE_ASSERT(newStart < m_reservedSize);

	const size_t newAllocatedSize = newStart + size;
	if (newAllocatedSize > m_reservedSize)
	{
		PHX_CORE_ASSERT(false, "Overflow Detected");
		return nullptr;
	}

	if (newAllocatedSize < m_commitedSize)
		return m_memory + newStart;


	Platform::Get().VirtualMemCommit(m_memory + m_commitedSize, size);
	m_commitedSize += newAllocatedSize;
	return m_memory + newStart;
}

void* PagedLinearAllocator::Allocate(size_t size, size_t alignment, const char* /*file*/, int32_t /*line*/)
{
	return Allocate(size, alignment);
}

void HeapAllocator::Initialize(size_t size)
{
	m_memory = std::malloc(size);
	m_maxSize = size;
	m_allocatedSize = 0;
	m_tlsfHandle = tlsf_create_with_pool(m_memory, size);
}

void HeapAllocator::Shutdown()
{
	// Check memory at the application exit.
	MemoryStatistics stats{ 0, m_maxSize };
	pool_t pool = tlsf_get_pool(m_tlsfHandle);

	tlsf_walk_pool(pool, ExitWalker, (void*)&stats);

	if (stats.AllocatedBytes)
	{
		PHX_CORE_ERROR(
			"HeapAllocator Shutdown FAILURE! Allocated memory detected. Allocated {0}, total {1}",
			stats.AllocatedBytes,
			stats.TotalBytes);
	}
	else {
		PHX_CORE_INFO("HeapAllocator Shutdown - all memory free!");
	}

	PHX_CORE_ASSERT(stats.AllocatedBytes == 0, "Allocations still present. Check your code!");

	tlsf_destroy(m_tlsfHandle);

	std::free(m_memory);
}

void* HeapAllocator::Allocate(size_t size, size_t alignment)
{
#if defined (HEAP_ALLOCATOR_STATS)
	void* allocatedMemory = alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
	sizet actualSize = tlsf_block_size(allocatedMemory);
	m_allocatedSize += actualSize;

	/*if ( size == 52224 ) {
		return allocatedMemory;
	}*/
	return allocatedMemory;
#else
	return alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
#endif // HEAP_ALLOCATOR_STATS
}

void* HeapAllocator::Allocate(size_t size, size_t alignment, const char*, int32_t)
{
	return Allocate(size, alignment);
}

void HeapAllocator::Deallocate(void* pointer)
{
#if defined (HEAP_ALLOCATOR_STATS)
	sizet actualSize = tlsf_block_size(pointer);
	m_allocatedSize -= actualSize;

	tlsf_free(m_tlsfHandle, pointer);
#else
	tlsf_free(m_tlsfHandle, pointer);
#endif
}

void StackAllocator::Initialize(size_t size)
{
	m_memory = static_cast<uint8_t*>(malloc(size));
	m_totalSize = size;
	m_allocatedSize = 0;
}

void StackAllocator::Shutdown()
{
	Clear();
	std::free(m_memory);
}

void* StackAllocator::Allocate(size_t size, size_t alignment)
{
	PHX_CORE_ASSERT(size > 0);

	const size_t newStart = AlignUp(m_allocatedSize, alignment);
	PHX_CORE_ASSERT(newStart < m_totalSize);

	const size_t newAllocatedSize = newStart + size;
	if (newAllocatedSize > m_totalSize)
	{
		PHX_CORE_ASSERT(false, "Overflow Detected");
		return nullptr;
	}

	m_allocatedSize += newAllocatedSize;
	return m_memory + newStart;
}

void* StackAllocator::Allocate(size_t size, size_t alignment, const char*, int32_t)
{
	return Allocate(size, alignment);
}

void StackAllocator::FreeMarker(size_t marker)
{
	const size_t difference = marker - m_allocatedSize;
	if (difference > 0)
		m_allocatedSize = marker;
}

void DoubleStackAllocator::Initialize(size_t size)
{
	m_memory = static_cast<uint8_t*>(malloc(size));
	m_totalSize = size;
	m_top = m_totalSize;
	m_bottom = 0;
}

void DoubleStackAllocator::Shutdown()
{
	ClearTop();
	ClearBottom();
	std::free(m_memory);
}


void* DoubleStackAllocator::AllocateTop(size_t size, size_t alignment)
{
	PHX_CORE_ASSERT(size > 0);

	const size_t new_start = AlignUp(m_top - size, alignment);
	if (new_start <= m_bottom)
	{
		PHX_CORE_ASSERT(false && "Overflow Crossing");
		return nullptr;
	}

	m_top = new_start;
	return m_memory + new_start;
}

void* DoubleStackAllocator::AllocateBottom(size_t size, size_t alignment)
{
	PHX_CORE_ASSERT(size > 0);

	const size_t new_start = AlignUp(m_bottom, alignment);
	if (new_start <= m_top)
	{
		PHX_CORE_ASSERT(false && "Overflow Crossing");
		return nullptr;
	}

	m_bottom = new_start;
	return m_memory + new_start;
}

void DoubleStackAllocator::DeallocateTop(size_t size)
{
	if (size > m_totalSize - m_top)
		m_top = m_totalSize;
	else
		m_top += size;
}

void DoubleStackAllocator::DeallocateBottom(size_t size)
{
	if (size > m_bottom)
		m_bottom = 0;
	else
		m_bottom -= size;
}

void DoubleStackAllocator::FreeMarkerTop(size_t marker)
{
	if (marker > m_top && marker < m_totalSize)
		m_top = marker;
}

void DoubleStackAllocator::FreeMarkerBottom(size_t marker)
{
	if (marker < m_bottom)
		m_bottom = marker;
}

void LinearAllocator::Initialize(size_t size)
{
	m_memory = static_cast<uint8_t*>(malloc(size));
	m_totalSize = size;
	m_allocatedSize = 0;
}

void LinearAllocator::Shutdown()
{
	Clear();
	std::free(m_memory);
}

void* LinearAllocator::Allocate(size_t size, size_t alignment)
{
	PHX_CORE_ASSERT(size > 0);

	const size_t newStart = AlignUp(m_allocatedSize, alignment);
	PHX_CORE_ASSERT(newStart < m_totalSize);

	const size_t newAllocatedSize = newStart + size;
	if (newAllocatedSize > m_totalSize)
	{
		PHX_CORE_ASSERT(false, "Overflow Detected");
		return nullptr;
	}

	m_allocatedSize += newAllocatedSize;
	return m_memory + newStart;
}

void* LinearAllocator::Allocate(size_t size, size_t alignment, const char*, int32_t)
{
	return Allocate(size, alignment);
}
#endif