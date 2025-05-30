#include "PhxCore_pch.h"

#include "PhxCore/Memory.h"

#include <tlsf.h>
#include <iostream>
#include <mutex>

using namespace phx;

namespace
{
	void ExitWalker(void* ptr, size_t size, int used, void* user)
	{
		MemoryStatistics* stats = (MemoryStatistics*)user;
		stats->Add(used ? size : 0);

		if (used)
			PHX_CORE_WARN("Found active allocation {0}, {1}\n", ptr, size);
	}
}

namespace phx
{
	namespace Memory
	{
		MallocAllocator g_systemAllocator;
		HeapAllocator g_persistentAllocator;
		StackAllocator g_frameScratchAllocator;
		LinearAllocator g_frameAllocator;

		void Initialize(MemoryDescriptor const& desc)
		{
			g_persistentAllocator.Initialize(desc.MaxPersistentHeapSize);
			g_frameAllocator.Initialize(desc.MaxFrameHeapSize);
			g_frameScratchAllocator.Initialize(desc.MaxScratchHeapSize);

			PHX_CORE_INFO("[Memory] Initialized Persistent Heap with {0} Mib", PhxToMB(desc.MaxPersistentHeapSize));
			PHX_CORE_INFO("[Memory] Initialized Frame Heap with {0} Mib", PhxToMB(desc.MaxFrameHeapSize));
			PHX_CORE_INFO("[Memory] Initialized Scratch Heap with {0} Mib", PhxToMB(desc.MaxScratchHeapSize));
		}

		void Finalize()
		{
			g_frameAllocator.Finalize();
			g_frameScratchAllocator.Finalize();
			g_persistentAllocator.Finalize();
		}


		void BeginFrame()
		{
			g_frameAllocator.Clear();
			g_frameAllocator.Clear();
		}


	}
}



void HeapAllocator::Initialize(size_t size)
{
	m_memory = std::malloc(size);
	m_maxSize = size;
	m_allocatedSize = 0;
	m_tlsfHandle = tlsf_create_with_pool(m_memory, size);
}

void HeapAllocator::Finalize()
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

	PHX_CORE_ASSERT(stats.AllocatedBytes== 0, "Allocations still present. Check your code!");

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

void* HeapAllocator::Allocate(size_t size, size_t alignment, const char* , int32_t )
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

void StackAllocator::Finalize()
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

void DoubleStackAllocator::Finalize()
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
	m_allocatedSize= 0;
}

void LinearAllocator::Finalize()
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

void* LinearAllocator::Allocate(size_t size, size_t alignment, const char* ,int32_t)
{
	return Allocate(size, alignment);
}

void* MallocAllocator::Allocate(size_t size, size_t)
{
	return malloc(size);
}

void* MallocAllocator::Allocate(size_t size, size_t, const char*, int32_t)
{
	return malloc(size);
}

void MallocAllocator::Deallocate(void* pointer)
{
	free(pointer);
}

namespace
{
	// OLD Virtual heap
#if false
	uint8_t* VirtualPtr;
	size_t TotalMemoryCommited = 0;
	size_t PtrOffset = 0;
	std::mutex Mutex;
	VirtualStackAllocator gFrameAllocator;
	VirtualStackAllocator gScratchAllocator;

	uint8_t* Commit(size_t commitSize)
	{
		std::scoped_lock _(Mutex);

		size_t originalOffset = PtrOffset;
		PtrOffset += commitSize;
		if (PtrOffset < TotalMemoryCommited)
		{
			// no need to commit more memory, return address offset
			return VirtualPtr + originalOffset;
		}

		// Commit data
		VirtualAlloc(VirtualPtr + originalOffset, commitSize, MEM_COMMIT, PAGE_READWRITE);
		TotalMemoryCommited += commitSize;

		return VirtualPtr + originalOffset;
	}


	class VirtualStackAllocator
	{
	public:
		struct Marker
		{
			size_t PageIndex;
			size_t ByteOffset;
		};

	public:
		VirtualStackAllocator(size_t pageSize = 4_MiB);

		template<typename T, typename... TArgs>
		[[nodiscard]] T* Alloc(TArgs&&... Args)
		{
			static_assert(std::is_trivially_destructible<T>::value, "Doesn't support Deconstrutable Types");

			void* memory = Allocate(sizeof(T), alignof(T));
			return new (memory) T(std::forward<TArgs>(Args)...);
		}

		template<typename T>
		[[nodiscard]] T* AllocArray(size_t count)
		{
			static_assert(std::is_trivially_destructible<T>::value, "Doesn't support Deconstrutable Types");

			return static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
		}

		template<typename T>
		[[nodiscard]] SpanMutable<T> AllocSpan(size_t count)
		{
			static_assert(std::is_trivially_destructible<T>::value, "Doesn't support Deconstrutable Types");

			T* ptr = static_cast<T*>(Allocate(sizeof(T) * count, alignof(T)));
			return SpanMutable<T>(ptr, count);
		}

		void* Allocate(size_t size, size_t alignment);

		void Reset();
		Marker GetMarker();
		void FreeMarker(Marker marker);

	private:
		const size_t m_pageSize;
		std::vector<uint8_t*> m_pages;
		size_t m_currentPage;
		size_t m_ptrOffset;

		std::mutex m_mutex;
	};

	VirtualStackAllocator::VirtualStackAllocator(size_t pageSize)
		: m_pageSize(pageSize)
		, m_currentPage(0)
		, m_ptrOffset(0)
	{
	}

	void* VirtualStackAllocator::Allocate(size_t size, size_t alignment)
	{
		std::scoped_lock _(this->m_mutex);

		size_t alignedSize = AlignUp(size, alignment);
		// If too big of an allocation then just allocate directly in the virtual allocator.
		if (alignedSize > m_pageSize)
		{
			// TODO: Warn
			return Commit(alignedSize);
		}

		// if there isn't enough space on the current page, request a new one
		if (this->m_pages.empty() || this->m_ptrOffset + alignedSize > this->m_pageSize)
		{
			m_currentPage += 1;
			if (m_currentPage >= this->m_pages.size())
			{
				// request a new page
				m_currentPage = this->m_pages.size();
				m_pages.push_back(Commit(this->m_pageSize));
			}

			this->m_ptrOffset = 0;
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

#endif

}