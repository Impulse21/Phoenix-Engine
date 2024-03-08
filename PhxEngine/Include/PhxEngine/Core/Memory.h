#pragma once

#include <stdint.h>
#include <array>
#include <mutex>
#include <vector>

#define PhxKB(size)                 (size * 1024)
#define PhxMB(size)                 (size * 1024 * 1024)
#define PhxGB(size)                 (size * 1024 * 1024 * 1024)

#define phx_new					new
#define phx_delete				delete

namespace PhxEngine
{
	constexpr uint8_t DefaultAligment = 16;

	inline size_t MemoryAlign(size_t size, size_t alignment) 
	{
		const size_t alignmentMask = alignment - 1;
		return (size + alignmentMask) & ~alignmentMask;
	}

	class IAllocator
	{
	public:
		virtual ~IAllocator() = default;

		virtual void* Allocate(size_t size, uint8_t alignment) = 0;
		virtual void Free(void* pointer) = 0;

		template <typename T, typename... args>
		T* New(args... argList)
		{
			void* mem = this->Allocate(sizeof(T), DefaultAligment);
			return new (mem) T(argList...);
		}

		template <typename T>
		T* NewArr(size_t length, uint8_t alignment) 
		{
			void* alloc = this->Allocate(sizeof(T) * length, alignment);
			char* allocAddress = static_cast<char*>(alloc);
			for (int i = 0; i < length; ++i) new (allocAddress + i * sizeof(T)) T;
			return static_cast<T*>(alloc);
		}
	};

	class StackAllocator final : public IAllocator
	{
	public:
		using Marker = size_t;

	public:
		StackAllocator() = delete;
		explicit StackAllocator(size_t stackSize);
		~StackAllocator();

		void* Allocate(size_t size, uint8_t alignment) override;
		void Free(void* pointer) override;

		size_t GetMarker() { return this->m_allocatedSize; }
		void FreeToMarker(Marker marker) { this->m_allocatedSize = marker; }

		void Clear() { this->m_allocatedSize = 0; }

	private:
		uint8_t* m_memory = nullptr;
		size_t m_totalSize = 0;
		size_t	m_allocatedSize = 0;
	};

	class DoubleBufferedAllocator : public IAllocator
	{
	public:
		DoubleBufferedAllocator() = delete;
		explicit DoubleBufferedAllocator(size_t stackSize);
		~DoubleBufferedAllocator() = default;

		void* Allocate(size_t size, uint8_t alignment) override;
		void Free(void* pointer) override;

		void SwapBuffers();
		void ClearCurrentBuffer();

	private:

		std::array<StackAllocator, 2> m_stacks;
		uint8_t m_curStackIndex;
	};

	class BitSetAllocator
	{
	public:
		explicit BitSetAllocator(size_t capacity);

		int Allocate();
		void Release(int index);

	private:
		int m_nextAvailable;
		std::vector<bool> m_allocated;
		std::mutex m_mutex;

	};
}

