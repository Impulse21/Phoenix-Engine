#pragma once

#include "memory"
#include <tuple>

#include <PhxCore/Span.h>
#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IO/MemoryRegion.h>

namespace phx
{
	using OffsetHandle = size_t;
	using OffsetHandle32 = uint32_t;
	constexpr OffsetHandle32 kInvalidOffset = ~0u;

	template<class TOffsetHandle = size_t>
	class BinaryBuilder
	{
	public:
		TOffsetHandle Reserve(size_t sizeInBytes, size_t alignment = 1)
		{
			uint32_t offset = MemoryAlign(m_totalSize, alignment);
			m_totalSize = offset + sizeInBytes;

			return offset;
		}

		template<typename T>
		TOffsetHandle Reserve(size_t alignment = alignof(T))
		{
			return Reserve(sizeof(T), alignment);
		}

		template<typename T>
		TOffsetHandle ReserveArray(size_t count, size_t alignment = alignof(T))
		{
			return Reserve(sizeof(T) * count, alignment);
		}

		size_t GetSize() const { return m_totalSize; }
		void Commit()
		{
			m_data = std::make_unique<std::byte[]>(m_totalSize);
		}

		void* Place(TOffsetHandle offset)
		{
			return m_data.get() + offset;
		}

		template<typename T>
		T* PlaceType(TOffsetHandle offset)
		{
			return reinterpret_cast<T*>(Place(offset));
		}

		Span<std::byte> GetMemory()
		{
			return { m_data.get(), m_totalSize };
		}

		MemoryBuffer Finalize()
		{
			const size_t final_size = m_totalSize;
			std::unique_ptr<std::byte[]> data_to_return = std::move(m_data);

			m_totalSize = 0;

			return MemoryBuffer(std::move(data_to_return), final_size);
		}

	private:
		inline size_t MemoryAlign(size_t size, size_t alignment)
		{
			const size_t alignmentMask = alignment - 1;
			return (size + alignmentMask) & ~alignmentMask;
		}
	private:
		// TODO: This could just become a blob.
		size_t m_totalSize = 0;
		std::unique_ptr<std::byte[]> m_data;
	};
}