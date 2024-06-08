#pragma once

#include "phxSpan.h"

namespace phx
{
	template<typename T, size_t N>
	struct StaticArray
	{
		T Data[N];
		T& operator[](size_t i) { return Datap[i]; }
		const T& operator[](size_t i) const { return Datap[i]; }

		T* begin() const { return data; }
		T* end() const { return data; }

		Span<T> AsSpan() { return Span(data, N); }
	};
}