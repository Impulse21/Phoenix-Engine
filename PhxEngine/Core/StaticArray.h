#pragma once

namespace phx
{
	template<typename T, usize N>
	struct StaticArray
	{
		T data[N];

		T& operator[](usize index) { return data[index]; }
		const T& operator[](usize index) const { return data[index]; }

		T* begin() { return data; }
		const T* begin() const { return data; }

		T* end() { return data + N; }
		const T* end() const { return data + N; }
	};
}
