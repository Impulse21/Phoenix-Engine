#include "pch.h"
#include "phxMemory.h"

#include <iostream>
#include <mutex>
#include <vector>


using namespace phx;

namespace
{
	uint8_t* VirtualPtr;
	size_t TotalMemoryCommited = 0;
	size_t PtrOffset = 0;
	std::mutex Mutex;
	VirtualStackAllocator gFrameAllocator;
	VirtualStackAllocator gScratchAllocator;;


	uint8_t* Commit(size_t commitSize)
	{
		std::scoped_lock _(Mutex);

		PtrOffset += commitSize;
		if (PtrOffset < TotalMemoryCommited)
		{
			// no need to commit more memory, return address offset
			return VirtualPtr + PtrOffset;
		}

		// Commit data
		VirtualAlloc(VirtualPtr + TotalMemoryCommited, commitSize, MEM_COMMIT, PAGE_READWRITE);
		TotalMemoryCommited += commitSize;
		PtrOffset += commitSize;
		return VirtualPtr + PtrOffset;
	}

}

void Memory::Initialize(MemoryConfiguration const& config)
{
	VirtualPtr = static_cast<uint8_t*>(VirtualAlloc(NULL, config.VirtualMemorySize, MEM_RESERVE, PAGE_READWRITE));
}

void Memory::Finalize()
{
	// Free the committed memory
	if (!VirtualFree(reinterpret_cast<void*>(VirtualPtr), 0, MEM_RELEASE))
	{
		std::cerr << "Memory deallocation failed." << std::endl;
	}
	VirtualPtr = nullptr;
	TotalMemoryCommited = 0;
	PtrOffset = 0;
}


void Memory::BeginFrame()
{
	std::scoped_lock _(Mutex);
	PtrOffset = 0;
	gFrameAllocator.Reset();
	gScratchAllocator.Reset();
}

VirtualStackAllocator& Memory::GetFrameAllocator()
{
	return gFrameAllocator;
}

VirtualStackAllocator& Memory::GetScratchAllocator()
{
	return gScratchAllocator;

}
VirtualStackAllocator::VirtualStackAllocator(size_t pageSize)
	: m_pageSize(pageSize)
	, m_currentPage(0)
	, m_ptrOffset(0)
{
}

void* VirtualStackAllocator::Allocate(size_t size, size_t alignment)
{
	std::scoped_lock _(this->m_mutex);

	size_t alignedSize = MemoryAlign(size, alignment);
	// If too big of an allocation then just allocate directly in the virtual allocator.
	if (alignedSize > m_pageSize)
	{
		// TODO: Warn
		return Commit(alignedSize);
	}

	// if there isn't enough space on the current page, request a new one
	if (this->m_pages.empty() || this->m_ptrOffset + alignedSize > this->m_pageSize)
	{
		// request a new page
		this->m_currentPage = this->m_pages.size();
		this->m_ptrOffset = 0;
		this->m_pages.push_back(Commit(this->m_pageSize));
	}

	void* ptr = this->m_pages[this->m_currentPage] + this->m_ptrOffset;
	this->m_ptrOffset += alignedSize;

	return ptr;
}

void VirtualStackAllocator::Reset()
{
	std::scoped_lock _(this->m_mutex);
	this->m_currentPage = 0;
	this->m_ptrOffset = 0;
}

VirtualStackAllocator::Marker VirtualStackAllocator::GetMarker()
{
	std::scoped_lock _(this->m_mutex);
	return { .PageIndex = this->m_currentPage, .ByteOffset = this->m_ptrOffset };
}

void VirtualStackAllocator::FreeMarker(VirtualStackAllocator::Marker marker)
{
	std::scoped_lock _(this->m_mutex);
	this->m_currentPage = marker.PageIndex;
	this->m_ptrOffset = marker.ByteOffset;
}