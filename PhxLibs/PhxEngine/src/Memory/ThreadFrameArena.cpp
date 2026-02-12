#include "PhxEngine_pch.h"

#include <PhxEngine/Memory/FrameMemoryManager.h>
#include <PhxEngine/Memory/ThreadFrameArena.h>
#include <PhxCore/Platform/PlatformWrapper.h>


using namespace phx;

void ThreadFrameArena::Initialize(size_t reserveBytes, size_t initialCommitBytes)
{
	m_baseAddress = static_cast<uint8_t*>(Platform::Get().VirtualMemReserve(reserveBytes));
	m_reservedSize = reserveBytes;

	m_pageSize = initialCommitBytes;

	uint8_t* ptr = m_baseAddress + m_allocatedSize;
	Platform::Get().VirtualMemCommit(ptr, m_pageSize);
	m_commitedSize = initialCommitBytes;

}
void ThreadFrameArena::Shutdown()
{
	Reset();

	if (!m_baseAddress)
		return;

	if (!Platform::Get().VirtualMemFree(m_baseAddress))
	{
		PHX_CORE_ERROR("Failed to free virtual memory");
	}

	m_baseAddress = nullptr;
	m_pageSize = 0;
	m_reservedSize = 0;
	m_commitedSize = 0;
}

void* ThreadFrameArena::Allocate(size_t size, size_t alignment)
{
	PHX_CORE_ASSERT(size > 0);

	const size_t newStart = AlignUp(m_allocatedSize, alignment);
	PHX_CORE_ASSERT(newStart < m_reservedSize);

	const size_t newAllocatedSize = newStart + size;
	if (newAllocatedSize > m_reservedSize)
	{
		PHX_CORE_ASSERT(false, "Overflow Detected");
		return nullptr;
	}

	if (newAllocatedSize <= m_commitedSize)
	{
		m_allocatedSize = newAllocatedSize;
		return m_baseAddress + newStart;
	}

	// -- Commit more memory ---
	size_t commitSize = m_pageSize;
	if (newAllocatedSize > m_pageSize)
		commitSize = AlignUp(size, m_pageSize);

	Platform::Get().VirtualMemCommit(m_baseAddress + m_commitedSize, commitSize);
	m_commitedSize += newAllocatedSize;

	m_allocatedSize = newAllocatedSize;
	return m_baseAddress + newStart;
}

void ThreadFrameArena::Deallocate(void* /*pointer*/)
{
}

void phx::ThreadFrameArena::Reset()
{
	m_allocatedSize = 0;
}

bool ThreadFrameArena::IsAddressInRange(const void* ptr) const
{
	if (!m_baseAddress || m_commitedSize == 0 || !ptr)
		return false;

	const uint8_t* p_char = static_cast<const uint8_t*>(ptr);
	const uint8_t* base_char = m_baseAddress;

	return (p_char >= base_char) && (p_char < (base_char + m_commitedSize));
}