#pragma once

#include <vector>
#include <PhxEngine/Core/Memory.h>
#include <mimalloc.h>

namespace PhxEngine::Core
{
#ifndef NOSTL
	template<typename T>
	using FlexArray = std::vector<T, mi_stl_allocator<T>>;
#else
	template<class T, class Alloc = DefaultAllocator>
	class FlexArray
	{
	public:
		FlexArray() = default;
		FlexArray(size_t initSize);
		~FlexArray() = default;


		void push_back(const T& element);

		template<typename... Args>
		T& emplace_back(Args&&... args)
		{
		}


		T& operator[](u32 index);
		const T& operator[](u32 index) const; 

		void clear();
		void resize(uint32_t newSize);
		void reserve(u32 newCapacity);

		T& back();
		const T& back() const;

		T& front();
		const T& front() const;

		size_t size_in_bytes() const;
		size_t capacity_in_bytes() const;

		size_t size() const;

	private:
		T* m_data;

	};
#endif
}