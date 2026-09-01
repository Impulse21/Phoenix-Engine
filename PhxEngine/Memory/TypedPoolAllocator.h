#pragma once

#include "IAllocator.h"
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace phx
{
	template <class T, size_t Size>
	class TypedPoolAllocator final : public IAllocator
	{
		static constexpr size_t BLOCK_SIZE =
			std::max(sizeof(T), sizeof(void*));

		static constexpr size_t TOTAL_SIZE =
			BLOCK_SIZE * Size;

		static constexpr size_t BLOCK_ALIGNMENT =
			std::max(alignof(T), alignof(void*));

	public:
		TypedPoolAllocator()
		{
			for (size_t i = 0; i < Size - 1; ++i)
			{
				void* next = &m_pool[(i + 1) * BLOCK_SIZE];

				std::memcpy(
					&m_pool[i * BLOCK_SIZE],
					&next,
					sizeof(next));
			}

			// Last block points to nullptr.
			void* null = nullptr;

			std::memcpy(
				&m_pool[(Size - 1) * BLOCK_SIZE],
				&null,
				sizeof(null));

			m_head = reinterpret_cast<T*>(&m_pool[0]);
		}

		void* Alloc(
			size_t size = sizeof(T),
			size_t alignment = alignof(T)) override
		{
			(void)size;
			(void)alignment;

			if (m_head == nullptr)
				return nullptr;

			T* result = m_head;

			void* next = nullptr;

			std::memcpy(
				&next,
				reinterpret_cast<const uint8_t*>(m_head),
				sizeof(next));

			m_head = static_cast<T*>(next);

			return result;
		}

		void* Alloc(
			size_t size,
			size_t alignment,
			const char* file,
			int32_t line)
		{
			(void)file;
			(void)line;

			return Alloc(size, alignment);
		}

		void Free(void* ptr) override
		{
			if (ptr == nullptr)
				return;

			T* node = static_cast<T*>(ptr);

			void* next = m_head;

			std::memcpy(
				reinterpret_cast<uint8_t*>(node),
				&next,
				sizeof(next));

			m_head = node;
		}

		bool IsAddressInRange(const void* ptr) const
		{
			if (ptr == nullptr)
				return false;

			const uint8_t* p =
				static_cast<const uint8_t*>(ptr);

			const uint8_t* base =
				m_pool;

			return p >= base &&
				   p < (base + TOTAL_SIZE);
		}

	private:
		alignas(BLOCK_ALIGNMENT)
		uint8_t m_pool[TOTAL_SIZE];

		T* m_head = nullptr;
	};
}