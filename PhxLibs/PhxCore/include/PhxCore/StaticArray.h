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
		const T* begin() const { return data; }

		T* end() { return data + N; }
		const T* end() const { return data + N; }
	};
}
