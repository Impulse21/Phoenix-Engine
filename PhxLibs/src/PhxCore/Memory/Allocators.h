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
}