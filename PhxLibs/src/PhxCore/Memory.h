#pragma once

#include <cstdint>
#include <mutex>
#include <vector>
#include "PhxCore/Base.h"

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

		using FrameAllocator = VirtualStackAllocator;
		FrameAllocator& GetFrameAllocator();

		using ScratchAllocator = VirtualStackAllocator;
		ScratchAllocator& GetScratchAllocator();

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
}
