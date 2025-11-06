#pragma once

namespace phx
{
	template<typename T, size_t N>
	struct StaticArray
	{
		T data[N];

		T& operator[](size_t index) { return data[index]; }
		const T& operator[](size_t index) const { return data[index]; }

		T* begin() { return data; }
		T* end() { return data + N; }
	};
}
