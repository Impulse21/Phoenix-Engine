#include <PhxCore/PhxCore_pch.h>
#include "Allocators.h"

using namespace phx;

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