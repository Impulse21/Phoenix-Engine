#pragma once

#include <PhxEngine/Core/Memory.h>
#include <memory>

namespace PhxEngine
{
	class BinaryBuilder
	{
	public:
		BinaryBuilder() = default;
		~BinaryBuilder() = default;

	public:
		template<typename T>
		size_t Reserve(size_t count = 1)
		{
			const size_t newStart = MemoryAlign(this->m_allocationSize, alignof(T));
			this->m_allocationSize = newStart + sizeof(T) * count;
			return newStart;
		}

		size_t Reserve(BinaryBuilder const& builder)
		{
			const size_t offset = this->m_allocationSize;
			this->m_allocationSize += builder.Size();

			return offset;
		}

		void Commit()
		{
			std::make_unique<char[]>(this->m_allocationSize);
		}

		template<typename T, typename... Args>
		T* Place(size_t offset, size_t numObjects = 1, Args&&... args)
		{
			for (int i = 0; i < numObjects; ++i)
			{
				new (m_memory.get() + offset * i) T(std::forward<Args>(args)...);
			}

			return  reinterpret_cast<T*>(this->m_memory.get() + offset);
		}

		template<typename T>
		T* Place(size_t offset, size_t numObjects = 1)
		{
			for (int i = 0; i < numObjects; ++i)
			{
				new (m_memory.get() + offset * i) T;
			}

			return  reinterpret_cast<T*>(this->m_memory.get() + offset);
		}


		void Place(size_t offset, BinaryBuilder const& builder)
		{
			void* startAddr = this->m_memory.get() + offset;
			std::memcpy(startAddr, builder.Data(), builder.Size());
		}

		const char* Data() const
		{
			return this->m_memory.get();
		}

		size_t Size() const
		{
			return this->m_allocationSize;
		}

	private:
		std::unique_ptr<char[]> m_memory;
		size_t m_allocationSize;
	};
}