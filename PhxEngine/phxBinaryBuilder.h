#pragma once

#include "memory"
#include "phxMemory.h"
#include "phxSpan.h"

namespace phx
{
	class BinaryBuilder
	{
	public:
		using OffsetHandle = size_t;
	public:

		OffsetHandle Reserve(size_t sizeInBytes, size_t alignment = 1)
		{
			uint32_t offset = MemoryAlign(m_totalSize, alignment);
			m_totalSize = offset + sizeInBytes;;

			return offset;
		}

		template<typename T>
		OffsetHandle Reserve(size_t alignment = alignof(T))
		{
			return Reserve(sizeof(T), alignment);
		}

		void Commit()
		{
			m_data = std::make_unique<uint8_t[]>(m_totalSize);
		}

		void* Place(OffsetHandle offset)
		{
			return m_data.get() + offset;
		}

		template<typename T>
		T* Place(OffsetHandle offset)
		{
			return reinterpret_cast<T*>(Place(offset));
		}

		Span<uint8_t> GetMemory()
		{
			return { m_data.get(), m_totalSize };
		}

	private:
		inline size_t MemoryAlign(size_t size, size_t alignment)
		{
			const size_t alignmentMask = alignment - 1;
			return (size + alignmentMask) & ~alignmentMask;
		}
	private:
		size_t m_totalSize = 0;
		std::unique_ptr<uint8_t[]> m_data;
	};
}