#pragma once

#include "memory"
#include <PhxCore/Span.h>
#include <PhxCore/IO/FileUtils.h>

namespace phx
{
	using OffsetHandle = size_t;
	using OffsetHandle32 = uint32_t;

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
			m_data = std::make_unique<uint8_t[]>(m_totalSize);
		}

		void* Place(TOffsetHandle offset)
		{
			return m_data.get() + offset;
		}

		template<typename T>
		T* Place(TOffsetHandle offset)
		{
			return reinterpret_cast<T*>(Place(offset));
		}

		Span<uint8_t> GetMemory()
		{
			return { m_data.get(), m_totalSize };
		}

		std::unique_ptr<IBlob> Finalize()
		{
			std::unique_ptr<IBlob> retVal = std::make_unique<Blob>(m_data.release(), m_totalSize);
			m_totalSize = 0;

			return retVal;
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
		std::unique_ptr<uint8_t[]> m_data;
	};
}