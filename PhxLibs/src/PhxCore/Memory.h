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

	namespace Memory
	{
		void Initialize(MemoryDescriptor const& desc);
		void Finalize();

		void BeginFrame();

		extern MallocAllocator g_SystemAllocator;
		extern HeapAllocator g_PersistentAllocator;
		extern StackAllocator g_frameScratchAllocator;
		extern LinearAllocator g_frameAllocator;
	}

	struct ScopedScratchMarker
	{
		size_t Marker;
		ScopedScratchMarker()
		{
			Marker = Memory::g_frameScratchAllocator.GetMarker();
		}

		~ScopedScratchMarker() { Memory::g_frameScratchAllocator.FreeMarker(Marker); }
	};
}

namespace phx 
{

	template<typename T>
	class unique_ptr
	{
	public:
		unique_ptr() = default;

		unique_ptr(T* ptr, IAllocator* allocator)
			: m_ptr(ptr), m_allocator(allocator)
		{
		}

		~unique_ptr()
		{
			reset();
		}

		unique_ptr(const unique_ptr&) = delete;
		unique_ptr& operator=(const unique_ptr&) = delete;

		unique_ptr(unique_ptr&& other) noexcept
			: m_ptr(other.m_ptr), m_allocator(other.m_allocator)
		{
			other.m_ptr = nullptr;
			other.m_allocator = nullptr;
		}

		unique_ptr& operator=(unique_ptr&& other) noexcept
		{
			if (this != &other)
			{
				reset();
				m_ptr = other.m_ptr;
				m_allocator = other.m_allocator;
				other.m_ptr = nullptr;
				other.m_allocator = nullptr;
			}
			return *this;
		}

		void reset()
		{
			if (m_ptr)
			{
				m_ptr->~T();
				m_allocator->deallocate(m_ptr);
				m_ptr = nullptr;
			}
		}

		T* get() const { return m_ptr; }
		T& operator*() const { return *m_ptr; }
		T* operator->() const { return m_ptr; }
		explicit operator bool() const { return m_ptr != nullptr; }

	private:
		T* m_ptr = nullptr;
		IAllocator* m_allocator = nullptr;
	};

	template<typename T>
	class shared_ptr
	{
	public:
		shared_ptr() = default;

		shared_ptr(T* ptr, IAllocator* allocator)
			: m_ptr(ptr), m_allocator(allocator)
		{
			if (ptr) m_refCount = new int(1);
		}

		~shared_ptr()
		{
			release();
		}

		shared_ptr(const shared_ptr& other)
			: m_ptr(other.m_ptr), m_allocator(other.m_allocator), m_refCount(other.m_refCount)
		{
			if (m_refCount) ++(*m_refCount);
		}

		shared_ptr& operator=(const shared_ptr& other)
		{
			if (this != &other)
			{
				release();
				m_ptr = other.m_ptr;
				m_allocator = other.m_allocator;
				m_refCount = other.m_refCount;
				if (m_refCount) ++(*m_refCount);
			}
			return *this;
		}

		void release()
		{
			if (m_refCount && --(*m_refCount) == 0)
			{
				m_ptr->~T();
				m_allocator->deallocate(m_ptr);
				delete m_refCount;
			}
			m_ptr = nullptr;
			m_refCount = nullptr;
		}

		T* get() const { return m_ptr; }
		T& operator*() const { return *m_ptr; }
		T* operator->() const { return m_ptr; }
		explicit operator bool() const { return m_ptr != nullptr; }

	private:
		T* m_ptr = nullptr;
		int* m_refCount = nullptr;
		IAllocator* m_allocator = nullptr;
	};

	template<typename T>
	class intrusive_ptr
	{
	public:
		intrusive_ptr() = default;

		intrusive_ptr(T* ptr)
			: m_ptr(ptr)
		{
			if (m_ptr) m_ptr->add_ref();
		}

		~intrusive_ptr()
		{
			if (m_ptr) m_ptr->release();
		}

		intrusive_ptr(const intrusive_ptr& other)
			: m_ptr(other.m_ptr)
		{
			if (m_ptr) m_ptr->add_ref();
		}

		intrusive_ptr& operator=(const intrusive_ptr& other)
		{
			if (this != &other)
			{
				if (m_ptr) m_ptr->release();
				m_ptr = other.m_ptr;
				if (m_ptr) m_ptr->add_ref();
			}
			return *this;
		}

		T* get() const { return m_ptr; }
		T& operator*() const { return *m_ptr; }
		T* operator->() const { return m_ptr; }
		explicit operator bool() const { return m_ptr != nullptr; }

	private:
		T* m_ptr = nullptr;
	};
} // namespace phx

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

template<typename T, AllocatorType Allocator>
T* phx_new_arr(Allocator& allocator, size_t count)
{
	const size_t headerCount = sizeof(size_t);
	const size_t totalSize = headerCount + sizeof(T) * count;
	void* mem = allocator.Allocate(totalSize, alignof(T));

	// Store the count at the beginning
	*reinterpret_cast<size_t*>(mem) = count;

	T* array = reinterpret_cast<T*>((char*)mem + headerSize);
	for (size_t i = 0; i < count; ++i)
		new (&array[i]) T();

	return array;

}

size_t phx_array_len(void* ptr)
{
	void* raw = (char*)ptr - sizeof(size_t);
	return *reinterpret_cast<size_t*>(raw);
}

template<typename T, AllocatorType Allocator>
void phx_delete_arr(Allocator& allocator, T* ptr)
{
	if (!ptr)
		return;

	const size_t count = phx_array_len(ptr);

	for (size_t i = 0; i < count; ++i)
		ptr[i].~T();

	allocator.Deallocate(raw);
};


#define phx_new_system(Type, ...) phx_new<Type>(phx::Memory::g_SystemAllocator, __VA_ARGS__)
#define phx_delete_system(Ptr) phx_delete(phx::Memory::g_SystemAllocator, Ptr)

#define phx_new_persistent(Type, ...) phx_new<Type>(phx::Memory::g_PersistentAllocator, __VA_ARGS__)
#define phx_delete_persistent(Ptr) phx_delete(phx::Memory::g_PersistentAllocator, Ptr)

#define phx_new_scratch(Type, ...) phx_new<Type>(phx::Memory::g_frameScratchAllocator, __VA_ARGS__)

#define phx_new_frame(Type, ...) phx_new<Type>(phx::Memory::g_frameAllocator, __VA_ARGS__)

// -- array varents ---
#define phx_new_arr_system(Type, Count) phx_new_arr<Type>(phx::Memory::g_SystemAllocator, Count)
#define phx_delete_arr_system(Ptr) phx_delete_arr(phx::Memory::g_SystemAllocator, Ptr)

#define phx_new_arr_persistent(Type, Count) phx_new_arr<Type>(phx::Memory::g_PersistentAllocator, Count)
#define phx_delete_arr_persistent(Ptr) phx_delete_arr(phx::Memory::g_PersistentAllocator, Ptr)


#define phx_new_scratch(Type, Count) phx_new_arr<Type>(phx::Memory::g_frameScratchAllocator, Count)

#define phx_new_frame(Type, Count) phx_new_arr<Type>(phx::Memory::g_frameAllocator, Count)