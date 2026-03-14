#pragma once

#include "IAllocator.h"

namespace phx
{
	class StackAllocator final : public IAllocator
	{
	public:
		~StackAllocator() override = default;

		void Initialize(size_t size);
		void Shutdown();

		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* /*pointer*/) override {};

		size_t GetMarker() { return m_allocatedSize; }
		void FreeMarker(size_t marker);
		bool IsAddressInRange(const void* ptr) const override;

		void Clear() { m_allocatedSize = 0; }

	private:
		uint8_t* m_memory = nullptr;
		size_t   	m_totalSize = 0;
		size_t		m_allocatedSize = 0;
	};

	class DoubleStackAllocator final : public IAllocator
	{
	public:
		~DoubleStackAllocator() override = default;

		void Initialize(size_t size);
		void Shutdown();

		void* Allocate(size_t /*size*/, size_t /*alignment*/) override { return nullptr; };
		void* Allocate(size_t /*size*/, size_t /*alignment*/, const char* /*file*/, int32_t /*line*/) override { return nullptr; };

		void Deallocate(void* /*pointer*/) override {};

		void* AllocateTop(size_t size, size_t alignment);
		void* AllocateBottom(size_t size, size_t alignment);

		void	DeallocateTop(size_t size);
		void	DeallocateBottom(size_t size);
		bool IsAddressInRange(const void* ptr) const override;

		size_t GetMarkerTop() { return m_top; }
		size_t GetMarkerButtom() { return m_bottom; }

		void FreeMarkerTop(size_t marker);
		void FreeMarkerBottom(size_t marker);

		void ClearTop() { m_top = m_totalSize; }
		void ClearBottom() { m_bottom = 0; }

	private:
		uint8_t* m_memory = nullptr;
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
		void Shutdown();

		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* /*pointer*/) override {};
		bool IsAddressInRange(const void* ptr) const override;

		void Clear()
		{
			m_allocatedSize = 0;
		}

	private:
		uint8_t* m_memory = nullptr;
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
		void Shutdown() {};

		void* Allocate(size_t size, size_t alignment) override;
		void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

		void Deallocate(void* pointer) override;
		bool IsAddressInRange(const void*) const override{ return true; }
	};

	template <class T, size_t Size>
	class TypedPoolAllocator final : public IAllocator
	{
		static constexpr size_t TOTAL_SIZE = sizeof(T) * Size; 
		static constexpr size_t BLOCK_SIZE = std::max(sizeof(T), sizeof(void*));
	public:
		TypedPoolAllocator()
		{
			for (size_t i = 0; i < Size - 1; ++i)
			{
            	void** current_block = (void**)&m_pool[i * BLOCK_SIZE];

            	*current_block = (void*)&m_pool[(i + 1) * BLOCK_SIZE];
			}

			// Point last element to nullptr
			*(T **)&m_pool[Size - 1] = nullptr;
			m_head = reinterpret_cast<T*>(&m_pool[0]);
		}

		void* Allocate(size_t size = sizeof(T), size_t alignment = alignof(T)) override
		{
			(void)size;
			(void)alignment;

			if (m_head == nullptr)
			{
				return nullptr;
			}

			T* res = m_head;
			m_head = *(T **)m_head;
			return res;
		}

		void* Allocate(size_t size, size_t alignment, const char*, int32_t) override
		{
			return Allocate(size, alignment);
		}

		void Deallocate(void *ptr) override
		{
			if (ptr == nullptr)
			{
				return;
			}

			// Push the block back onto the free list
			T *node = (T *)ptr;
			*(T **)node = m_head;
			m_head = node;
		}

		bool IsAddressInRange(const void* ptr) const override
		{ 
			if (!ptr)
				return false;
				
			const uint8_t* p_char = static_cast<const uint8_t*>(ptr);
			const uint8_t* base_char = &m_pool[0];
			return (p_char >= base_char) && (p_char < (base_char + TOTAL_SIZE));
		}

	private:
		alignas(T) uint8_t m_pool[TOTAL_SIZE];
		T *m_head = nullptr;
	};
	
	// -- not sure about this
	#if false
	namespace mem
	{
		template<class T, typename... Args>
		T* Create(IAllocator& allocator, Args&&... args)
		{
			void* mem = allocator->Allocate(sizeof(T), alignof(T));
        	if (!mem) 
				return nullptr;

        	return new (mem) T(std::forward<Args>(args)...);
		}

		template<typename T>
    	void Destroy(T* ptr)
    	{
        	if (!ptr) 
				return;
				
        	ptr->~T();

        	Deallocate(ptr); // Return memory to pool
    	}
	}
	#endif
}