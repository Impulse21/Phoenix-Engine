#pragma once

#include <initializer_list>
#include <vector>
#include <PhxEngine/Core/Asserts.h>

namespace PhxEngine::Core
{
	// This class was created after seeing a post by Sebastian
	// https://twitter.com/SebAaltonen/status/1502640283502227465?s=20&t=021wmMNR3266F42kZiIkSA

	// Note: STL has an implementation in C++20
	// https://en.cppreference.com/w/cpp/numeric/valarray/slice

	template<typename T>
	class Slice
	{
	public:
		Slice(T* array, size_t length)
			: Slice(array, length, 0)
		{}

		Slice(std::initializer_list<T> v)
			: Slice(v.begin(), v.size, 0)
		{};

		Slice(std::vector<T>& v)
			: Slice(v.data(), v.size(), 0)
		{}

		Slice(T* array, size_t length, size_t skip)
			: m_array(array + skip)
			, m_length(length)
		{
		}


		T& operator[](size_t index)
		{
			PHX_ASSERT(index < this->m_length);
			return this->m_array[index];
		}

		size_t Size() const { return this->m_length; }
		T* Begin() { return this->m_array; }
		T* End() { return this->m_array + this->m_length };

		const T* Begin() const { return this->m_array; }
		const T* End() const { return this->m_array + this->m_length };

	private:
		T* m_array;
		size_t m_length;

	};

}