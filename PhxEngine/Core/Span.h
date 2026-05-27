#pragma once

#include <initializer_list>
#include <vector>
#include <assert.h>
#include <PhxEngine/Core/EnumUtils.h>
#include <PhxEngine/Core/StaticArray.h>


namespace phx
{
	// This class was created after seeing a post by Sebastian
	// https://twitter.com/SebAaltonen/status/1502640283502227465?s=20&t=021wmMNR3266F42kZiIkSA

	// Note: STL has an implementation in C++20
	// https://en.cppreference.com/w/cpp/numeric/valarray/slice


	template<typename T>
	class SpanMutable
	{
	public:
		SpanMutable()
			: SpanMutable(nullptr, 0)
		{ }

		SpanMutable(T* array, size_t length)
			: SpanMutable(array, length, 0)
		{}

		SpanMutable(std::vector<T>& v)
			: SpanMutable(v.data(), v.size(), 0)
		{}

		template <typename U, std::size_t N>
		SpanMutable(StaticArray<U, N>& arr)
			: SpanMutable(arr.begin(), N, 0)
		{
		}

		SpanMutable(T* array, size_t length, size_t skip)
			: m_array(array + skip)
			, m_length(length)
		{
		}

		template<typename E>
		SpanMutable(EnumArray<T, E>& a)
			: SpanMutable(a.data(), a.size(), 0)
		{}

		T& operator[](size_t index)
		{
			assert(index < this->m_length);
			return this->m_array[index];
		}

		T* data() { return this->m_array; }
		size_t Size() const { return this->m_length; }

		// -- Use lower case here so we can use the for each loop ---
		T* begin() const { return this->m_array; }
		T* end() const { return this->m_array + this->m_length; }

		bool IsEmpty() const { return this->m_length == 0; }

	private:
		T* m_array;
		size_t m_length;
	};

	template<typename T>
	class Span
	{
	public:
		constexpr Span()
			: Span(nullptr, 0)
		{
		}

		Span(SpanMutable<T>& mutable_span)
			: Span(mutable_span.data(), mutable_span.Size())
		{

		}
		Span(const T* array, size_t length)
			: Span(array, length, 0)
		{
		}

		constexpr Span(std::initializer_list<T> v)
			: Span(v.begin(), v.size(), 0)
		{
		}

		Span(std::vector<T> const& v)
			: Span(v.data(), v.size(), 0)
		{
		}

		template <typename U, std::size_t N>
		Span(const std::array<U, N>& arr)
			: Span(arr.data(), arr.size(), 0)
		{
		}

		template <typename U, std::size_t N>
		Span(const StaticArray<U, N>& arr)
			: Span(arr.begin(), N, 0)
		{
		}

		template<typename E>
		Span(EnumArray<T, E>& a)
			: Span(a.data(), a.size(), 0)
		{
		}

		template <typename U, std::size_t N>
		constexpr Span(const U(&arr)[N]) noexcept
			: m_array(arr)
			, m_length(N)
		{
		}

		Span(const T* array, size_t length, size_t skip)
			: m_array(array + skip)
			, m_length(length)
		{
		}

		const T& operator[](size_t index)
		{
			assert(index < this->m_length);
			return this->m_array[index];
		}
		const T& operator[](size_t index) const
		{
			assert(index < this->m_length);
			return this->m_array[index];
		}

		const T* data() const { return this->m_array; }
		size_t size() const { return this->m_length; }
		size_t Size() const { return this->m_length; }

		// -- Use lower case here so we can use the for each loop ---
		const T* begin() const { return this->m_array; }
		const T* end() const { return this->m_array + this->m_length; }

		bool IsEmpty() const { return this->m_length == 0; }
	private:
		const T* m_array;
		size_t m_length;
	};
}