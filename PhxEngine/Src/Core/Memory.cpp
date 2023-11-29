#include <assert.h>
#include <PhxEngine/Core/Memory.h>
#include <tlsf.h>
#include <mimalloc.h>
#include <atomic>

#define HEAP_ALLOCATOR_STATS

using namespace PhxEngine;
using namespace PhxEngine::Core;

void* operator new(size_t p_size, const char* p_description) 
{
	return SystemMemory::Alloc(p_size, false);
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
	constexpr size_t kPadding = 16;
	std::atomic_uint64_t m_allocCount = 0;
	std::atomic_uint64_t m_memUsage = 0;
	std::atomic_uint64_t m_maxUsage = 0;

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

void* PhxEngine::Core::SystemMemory::Alloc(size_t bytes, bool padAlign)
{
	// Zeros memory
	void* mem = mi_zalloc(bytes);

	m_allocCount.fetch_add(1, std::memory_order_relaxed);
	assert(mem);

#ifdef _DEBUG
	uint64_t newMemUsage = m_memUsage.fetch_add(bytes);
	uint64_t current = m_memUsage.load();
	m_maxUsage.compare_exchange_strong(current, newMemUsage);
#endif

	return mem;
}

void* PhxEngine::Core::SystemMemory::AllocArray(size_t size, size_t count, bool padAlign)
{
	void* mem = mi_calloc(count, size);

	m_allocCount.fetch_add(1, std::memory_order_relaxed);
	assert(mem);

#ifdef _DEBUG
	size_t bytes = mi_malloc_size(mem);
	uint64_t newMemUsage = m_memUsage.fetch_add(bytes);
	uint64_t current = m_memUsage.load();
	m_maxUsage.compare_exchange_strong(current, newMemUsage);
#endif

	return mem;
}

void* PhxEngine::Core::SystemMemory::Realloc(void* memory, size_t bytes, bool padAlign)
{

#ifdef _DEBUG
	size_t previousSize = mi_malloc_size(memory);
#endif

	void* newMem = mi_realloc(memory, bytes);
	assert(newMem);

#ifdef _DEBUG
	m_memUsage.fetch_sub(previousSize);
	uint64_t newMemUsage = m_memUsage.fetch_add(bytes);

	uint64_t current = m_memUsage.load();
	m_maxUsage.compare_exchange_strong(current, newMemUsage);
#endif

	return newMem;
}

void* PhxEngine::Core::SystemMemory::ReallocArray(void* memory, size_t size, size_t newCount, bool padAlign)
{
#ifdef _DEBUG
	size_t previousSize = mi_malloc_size(memory);
#endif

	void* newMem = mi_reallocn(memory, newCount, size);
	assert(newMem);

#ifdef _DEBUG
	m_memUsage.fetch_sub(previousSize);
	size_t bytes = mi_malloc_size(newMem);
	uint64_t newMemUsage = m_memUsage.fetch_add(bytes);

	uint64_t current = m_memUsage.load();
	m_maxUsage.compare_exchange_strong(current, newMemUsage);
#endif

	return newMem;
}

void PhxEngine::Core::SystemMemory::Free(void* ptr, bool padAlign)
{
#ifdef _DEBUG
	size_t size = mi_malloc_size(ptr);
#endif

	mi_free(ptr);
#ifdef _DEBUG
	m_memUsage.fetch_sub(size);
#endif
}

void PhxEngine::Core::SystemMemory::Cleanup()
{
	mi_collect(true);
}

uint64_t PhxEngine::Core::SystemMemory::GetMemoryAvailable()
{
	return ~0lu;
}

uint64_t PhxEngine::Core::SystemMemory::GetMemUsage()
{
	return m_memUsage;
}

uint64_t PhxEngine::Core::SystemMemory::GetMemMaxUsage()
{
	return m_maxUsage;
}

uint64_t PhxEngine::Core::SystemMemory::GetNumActiveAllocations()
{
	return m_allocCount;
}

void PhxEngine::Core::HeapAllocator::Initialize(size_t size)
{
	this->m_memory = mi_malloc(size);
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
	mi_free(this->m_memory);
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

void PhxEngine::Core::HeapAllocator::Free(void* pointer)
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
	this->m_memory = static_cast<uint8_t*>(mi_malloc(size));
	this->m_totalSize = size;
	this->m_allocatedSize = 0;
}

void PhxEngine::Core::StackAllocator::Finalize()
{
	mi_free(this->m_memory);
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

void PhxEngine::Core::StackAllocator::Free(void* pointer)
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
	this->m_memory = static_cast<uint8_t*>(mi_malloc(size));
	this->m_totalSize = size;
	this->m_allocatedSize = 0;
}

void PhxEngine::Core::LinearAllocator::Finalize()
{
	this->Clear();
	mi_free(this->m_memory);
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

void PhxEngine::Core::LinearAllocator::Free(void* pointer)
{
	// NoOp
}

void PhxEngine::Core::LinearAllocator::Clear()
{
	this->m_allocatedSize = 0;
}