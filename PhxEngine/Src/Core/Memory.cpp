#include <assert.h>
#include <PhxEngine/Core/Memory.h>
#include <tlsf.h>

#define HEAP_ALLOCATOR_STATS

using namespace PhxEngine;
using namespace PhxEngine::Core;

void* operator new(size_t p_size, const char* p_description) 
{
	return SystemMemory::GetAllocator().Allocate(p_size, 1);
}

void* operator new(size_t p_size, void* (*p_allocfunc)(size_t p_size, size_t alignment)) 
{
	return p_allocfunc(p_size, 1);
}

#ifdef _MSC_VER
void operator delete(void* p_mem, const char* p_description) 
{
	assert(0);
}

void operator delete(void* p_mem, void* (*p_allocfunc)(size_t p_size)) 
{
	assert(0);
}

void operator delete(void* p_mem, void* p_pointer, size_t check, const char* p_description) 
{
	assert(0);
}

#endif

namespace
{
	HeapAllocator m_systemAllocator;

	void ExitWalker(void* ptr, size_t size, int used, void* user)
	{
		MemoryStatistics* stats = (MemoryStatistics*)user;
		stats->add(used ? size : 0);

	}

	size_t Align(size_t size, size_t alignment)
	{
		const size_t alignmentMask = alignment - 1;
		return (size + alignmentMask) & ~alignmentMask;
	}

}


void PhxEngine::Core::HeapAllocator::Initialize(size_t size)
{
	this->m_memory = malloc(size);
	this->m_maxSize = size;
	this->m_allocatedSize = 0;
	this->m_tlsfHandle = tlsf_create_with_pool(this->m_memory, size);
}

void PhxEngine::Core::HeapAllocator::Finalize()
{
	// Check Memory
	MemoryStatistics stats = {
		.AllocatedBytes = 0,
		.TotalBytes = this->m_maxSize,
		.AllocationCount = 0 };

	pool_t pool = tlsf_get_pool(this->m_tlsfHandle);
	tlsf_walk_pool(pool, ExitWalker, static_cast<void*>(&stats));

	// TODO: Add logging
	assert(stats.AllocatedBytes == 0);

	tlsf_destroy(this->m_tlsfHandle);
	free(this->m_memory);
}

void PhxEngine::Core::HeapAllocator::BuildIU()
{
	// TODO:
}

void* PhxEngine::Core::HeapAllocator::Allocate(size_t size, size_t alignment)
{
#ifdef HEAP_ALLOCATOR_STATS
	void* allocatedMemory = alignment == 1 ? tlsf_malloc(this->m_tlsfHandle, size) : tlsf_memalign(this->m_tlsfHandle, alignment, size);
	size_t actualSize= tlsf_block_size(allocatedMemory);
	this->m_allocatedSize += actualSize;

	return allocatedMemory;

#else
	return tlsf_malloc(this->m_tlsfHandle, size);
#endif // HEAP_ALLOCATOR_STATS
}

void* PhxEngine::Core::HeapAllocator::Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum)
{
	return this->Allocate(size, alignment);
}

void PhxEngine::Core::HeapAllocator::Deallocate(void* pointer)
{
#ifdef HEAP_ALLOCATOR_STATS
	size_t actualSize = tlsf_block_size(pointer);
	this->m_allocatedSize -= actualSize;

	tlsf_free(this->m_tlsfHandle, pointer);
#else
	tlsf_free(this->m_tlsfHandle, pointer);
#endif
}

void PhxEngine::Core::StackAllocator::Initialize(size_t size)
{
	this->m_memory = static_cast<uint8_t*>(malloc(size));
	this->m_totalSize = size;
	this->m_allocatedSize = 0;
}

void PhxEngine::Core::StackAllocator::Finalize()
{
	free(this->m_memory);
}

void* PhxEngine::Core::StackAllocator::Allocate(size_t size, size_t alignment)
{
	assert(size > 0);
	const size_t newStart = Align(size, alignment);
	assert(newStart < this->m_totalSize);

	const size_t newAllocatedSize = newStart + size;
	if (newAllocatedSize > this->m_totalSize)
	{
		assert(false);
		// report overflow
		return nullptr;
	}

	this->m_allocatedSize = newAllocatedSize;
	return this->m_memory + newStart;
}

void* PhxEngine::Core::StackAllocator::Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum)
{
	return this->Allocate(size, alignment);
}

void PhxEngine::Core::StackAllocator::Deallocate(void* pointer)
{
	assert(pointer > this->m_memory);
	assert(pointer < this->m_memory + this->m_totalSize);
	assert(pointer < this->m_memory + this->m_allocatedSize);

	const size_t sizeAtPointer = static_cast<uint8_t*>(pointer) - this->m_memory;

	this->m_allocatedSize = sizeAtPointer;
}

size_t PhxEngine::Core::StackAllocator::GetMarker()
{
	return this->m_allocatedSize;
}

void PhxEngine::Core::StackAllocator::FreeMarker(size_t marker)
{
	const size_t difference = marker - this->m_allocatedSize;
	if (difference > 0)
	{
		this->m_allocatedSize = marker;
	}
}

void PhxEngine::Core::StackAllocator::Clear()
{
	this->m_allocatedSize = 0;
}

void PhxEngine::Core::LinearAllocator::Initialize(size_t size)
{
	this->m_memory = static_cast<uint8_t*>(malloc(size));
	this->m_totalSize = size;
	this->m_allocatedSize = 0;
}

void PhxEngine::Core::LinearAllocator::Finalize()
{
	this->Clear();
	free(this->m_memory);
}

void* PhxEngine::Core::LinearAllocator::Allocate(size_t size, size_t alignment)
{
	assert(size > 0);

	const size_t newStart = Align(this->m_allocatedSize, alignment);
	assert(newStart < this->m_totalSize);
	const size_t newAllocatedSize = newStart + size;
	if (newAllocatedSize > this->m_totalSize)
	{
		assert(false);
		return nullptr;
	}

	this->m_allocatedSize = newAllocatedSize;

	return this->m_memory + newStart;
}

void* PhxEngine::Core::LinearAllocator::Allocate(size_t size, size_t alignment, std::string file, int32_t lineNum)
{
	return this->Allocate(size, alignment);
}

void PhxEngine::Core::LinearAllocator::Deallocate(void* pointer)
{
	// NoOp
}

void PhxEngine::Core::LinearAllocator::Clear()
{
	this->m_allocatedSize = 0;
}

void PhxEngine::Core::SystemMemory::Initialize(PhxEngine::Core::MemoryServiceConfiguration const& config)
{
	m_systemAllocator.Initialize(config.MaximumDynamicSize);
}

void PhxEngine::Core::SystemMemory::Finalize()
{
	m_systemAllocator.Finalize();
}

IAllocator& PhxEngine::Core::SystemMemory::GetAllocator() { return m_systemAllocator; }
