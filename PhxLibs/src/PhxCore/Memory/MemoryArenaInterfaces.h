#pragma once

#include <new>     // For std::align_val_t, std::bad_alloc
#include "IAllocator.h"

namespace phx
{
    // --- Main Arena Interface ---
    class MainArena final : public IAllocator
    {
    public:
		MainArena() = default;
		~MainArena() = default; // Proper cleanup in shutdown()

        void Initialize(size_t reserveBytes, size_t initialCommitBytes);
        void Shutdown();

        [[nodiscard]] void* Allocate(size_t size, size_t alignment) override;
        [[nodiscard]] void* Allocate(size_t size, size_t alignment, const char*, int32_t) override
        {
            return Allocate(size, alignment);
        }

        void Deallocate(void* pointer) override;

        bool IsAddressInRange(const void* ptr) const override;
        void* GetBaseAddress() const { return m_baseAddress; }
        size_t GetReservedSize() const { return m_reservedSize; }

    private:
        void* m_tlsfHandle;
        void* m_baseAddress = nullptr;

        size_t m_reservedSize = 0;
        size_t m_commitedSize = 0;
		size_t m_pageSize = 0;
		std::mutex m_mutex;
    };

    // --- Thread Frame Arena Interface ---
    class ThreadFrameArena final : public IAllocator
    { 
    public:
		ThreadFrameArena() = default;
		~ThreadFrameArena() = default;

        void Initialize(size_t reserveBytes, size_t initialCommitBytes);
        void Shutdown();

        [[nodiscard]] void* Allocate(size_t size, size_t alignment) override;
        [[nodiscard]] void* Allocate(size_t size, size_t alignment, const char*, int32_t) override
        {
            return Allocate(size, alignment);
        }

        void Deallocate(void* pointer) override;

        void Reset();
        bool IsAddressInRange(const void* ptr) const override;
        void* GetBaseAddress() const { return m_baseAddress; }
        size_t GetReservedSize() const { return m_reservedSize; }
        size_t GetCommitedSize() const { return m_commitedSize; }

        uint8_t* m_baseAddress = nullptr;
		size_t m_allocatedSize = 0;
        size_t m_reservedSize = 0;
        size_t m_commitedSize = 0;
		size_t m_pageSize = 0;
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

#if false // OLD Allocators

class HeapAllocator final : public IAllocator
{
public:
	~HeapAllocator() override = default;

	void Initialize(size_t size);
	void Shutdown();

	void* Allocate(size_t size, size_t alignment) override;
	void* Allocate(size_t size, size_t alignment, const char* file, int32_t line) override;

	void Deallocate(void* pointer) override;

private:
	void* m_tlsfHandle;
	void* m_memory = nullptr;
	size_t  m_allocatedSize = 0;
	size_t 	m_maxSize = 0;
};

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
};

#endif