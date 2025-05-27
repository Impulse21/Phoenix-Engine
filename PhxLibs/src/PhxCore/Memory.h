#pragma once

#include <cstdint>
#include <vector>
#include "PhxCore/Base.h"
#include "PhxCore/Span.h"

namespace phx
{
	template<typename T, typename U>
	constexpr T AlignUp(T Size, U Alignment)
	{
		return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
	}

	void* VirtualMemReserve(size_t reserveSize);

	template<typename T, size_t _PageSize = 1>
	T* VirtualMemReserveTyped(size_t numEntries)
	{
		void* alloc = VirtualMemReserve(AlignUp(numEntries * sizeof(T), _PageSize));
		return static_cast<T*>(alloc);
	}

	void VirtualMemCommit(void* ptr, size_t commitSize);
	bool VirtualMemFree(void* ptr);

	struct MemoryStatistics
	{
		size_t AllocatedBytes = 0;
		size_t TotalBytes = 0;
		size_t AllocationCount = 0;

		void Add(size_t a)
		{
			if (a)
			{
				AllocatedBytes += a;
				AllocationCount++;
			}
		}
	};

	struct NonCopyable
	{
		NonCopyable & operator=(const NonCopyable&) = delete;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable() = default;
	};

	class IAllocator : NonCopyable
	{
	public:
		virtual ~IAllocator() = default;

		virtual void* Allocate(size_t size, size_t alignment) = 0;
		virtual void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) = 0;

		virtual void Deallocate(void* pointer) = 0;
	};

	class HeapAllocator final : public IAllocator
	{
	public:
		~HeapAllocator() override = default;

		void Initialize(size_t size);
		void Finalize();
		
		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* pointer) override;

	private:
        void*	m_tlsfHandle;
        void*   m_memory = nullptr;
        size_t  m_allocatedSize = 0;
        size_t 	m_maxSize = 0;
	};

	class StackAllocator final : public IAllocator
	{
	public:
		~StackAllocator() override = default;

		void Initialize(size_t size);
		void Finalize();
		
		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* /*pointer*/) override {};

		size_t GetMarker() { return m_allocatedSize; }
		void FreeMarker(size_t marker);

		void Clear() { m_allocatedSize = 0; }

	private:
        uint8_t*	m_memory = nullptr;
        size_t   	m_totalSize = 0;
        size_t		m_allocatedSize = 0;
	};

	class DoubleStackAllocator final : public IAllocator
	{
	public:
		~DoubleStackAllocator() override = default;

		void Initialize(size_t size);
		void Finalize();
		
		void* Allocate(size_t /*size*/, size_t /*alignment*/) override { return nullptr; };
		void* Allocate(size_t /*size*/, size_t /*alignment*/, const char* /*file*/, int32_t /*line*/) override { return nullptr; };

		void Deallocate(void* /*pointer*/) override {};

        void*	AllocateTop(size_t size, size_t alignment);
        void*   AllocateBottom(size_t size, size_t alignment);

        void	DeallocateTop(size_t size);
        void	DeallocateBottom(size_t size);

		size_t GetMarkerTop() { return m_top; }
		size_t GetMarkerButtom() { return m_bottom; }

		void FreeMarkerTop(size_t marker);
		void FreeMarkerBottom(size_t marker);

		void ClearTop() { m_top = m_totalSize; }
		void ClearBottom() { m_bottom = 0; }

	private:
        uint8_t*	m_memory = nullptr;
        size_t   	m_totalSize = 0;
        size_t		m_top = 0;
        size_t		m_bottom = 0;
	};

	//
	// Allocator that can only be reset.
	//
	class LinearAllocator final : public IAllocator
	{
	public:
		~LinearAllocator() override = default;

		void Initialize(size_t size);
		void Finalize();
		
		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* /*pointer*/) override {};

		void Clear()
		{
			m_allocatedSize = 0;
		}

	private:
		uint8_t*	m_memory = nullptr;
		size_t		m_totalSize = 0;
		size_t 		m_allocatedSize = 0;
	};

	//
	// DANGER: this should be used for NON runtime processes, like compilation of resources.
	class MallocAllocator  final : public IAllocator
	{
	public:
		~MallocAllocator() override = default;

		void Initialize(size_t /*size*/) {};
		void Finalize() {};
		
		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* pointer) override;
	};

  	// Memory Service /////////////////////////////////////////////////////
	// 
	// 
	struct MemoryDescriptor
	{
		size_t MaxDynamicSize = 32_MiB;
		size_t MaxFrameSize = 32_MiB;
		size_t MaxScratchSize = 8_MiB;

		size_t VirtualMemorySize = 16_GiB;
		size_t StackPageSize = 4_MiB;
	};

	namespace MemoryService
	{
		void Initialize(MemoryDescriptor const& desc);
		void Finalize();

		void BeginFrame();

		extern HeapAllocator g_SystemAllocator;
		extern StackAllocator g_frameScratchAllocator;
		extern LinearAllocator g_frameAllocator;
	}

	struct ScopedScratchMarker
	{
		size_t Marker;
		ScopedScratchMarker()
		{
			Marker = MemoryService::g_frameScratchAllocator.GetMarker();
		}

		~ScopedScratchMarker() { MemoryService::g_frameScratchAllocator.FreeMarker(Marker); }
	};
}

template<typename T>
concept AllocatorType = std::is_base_of_v<phx::IAllocator, T>;

template<typename T, AllocatorType Allocator, typename... Args>
T* phx_new(Allocator& allocator, Args&&... args)
{
    void* mem = allocator.Allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
};

template<typename T, AllocatorType Allocator>
void phx_delete(Allocator& allocator, T* ptr)
{
    if (ptr)
    {
        ptr->~T();
        allocator.Deallocate(ptr);
    }
};

#define phx_new_system(Type, ...) phx_new<Type>(phx::MemoryService::g_SystemAllocator, __VA_ARGS__)
#define phx_delete_system(Ptr) phx_delete(phx::MemoryService::g_SystemAllocator, Ptr)

#define phx_new_scratch(Type, ...) phx_new<Type>(phx::MemoryService::g_frameScratchAllocator, __VA_ARGS__)

#define phx_new_frame(Type, ...) phx_new<Type>(phx::MemoryService::g_frameAllocator, __VA_ARGS__)

#define phx_new_heap(Type, ...) phx_new<Type>(phx::MallocAllocator(), __VA_ARGS__)
#define phx_delete_heap(Ptr) phx_delete(phx::MallocAllocator(), Ptr)