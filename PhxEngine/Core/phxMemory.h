#pragma once

#include <mutex>

#include "Core/phxSpan.h"

constexpr inline unsigned long long operator "" _KiB(unsigned long long value)
{
	return value << 10;
}

constexpr inline unsigned long long operator "" _MiB(unsigned long long value)
{
	return value << 20;
}

constexpr inline unsigned long long operator "" _GiB(unsigned long long value)
{
	return value << 30;
}

namespace phx
{
	inline size_t MemoryAlign(size_t size, size_t alignment)
	{
		const size_t alignmentMask = alignment - 1;
		return (size + alignmentMask) & ~alignmentMask;
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
		[[nodiscard]] phx::Span<T> AllocSpan(size_t count)
		{
			T* ptr = this->AllocArray(count);
			return Span<T>(ptr, count);
		}

		template<typename T>
		[[nodiscard]] phx::SpanMutable<T> AllocSpanMutable(size_t count)
		{
			T* ptr = this->AllocArray(count);
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

	namespace Memory
	{
		struct MemoryConfiguration
		{
			size_t VirtualMemorySize = 16_GiB;
			size_t StackPageSize = 4_MiB;
		};

		void Initialize(MemoryConfiguration const& config);
		void Finalize();
		void BeginFrame();

		VirtualStackAllocator& GetFrameAllocator();
		VirtualStackAllocator& GetScratchAllocator();

	}

	struct ScopedScratchMarker
	{
		VirtualStackAllocator::Marker Marker;
		ScopedScratchMarker()
		{
			this->Marker = Memory::GetScratchAllocator().GetMarker();
		}

		~ScopedScratchMarker() { Memory::GetScratchAllocator().FreeMarker(Marker); }
	};

#define BEGIN_TEMP_SCOPE ScopedScratchMarker _;
}