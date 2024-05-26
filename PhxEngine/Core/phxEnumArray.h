#pragma once

#include <array>

namespace phx::core
{
	template<typename E, class T, std::size_t N = (std::size_t)E::Count>
	class EnumArray : public std::array<T, N>
	{
	public:
		T& operator[] (E e)
		{
			return std::array<T, N>::operator[]((std::size_t)e);
		}

		const T& operator[] (E e) const
		{
			return std::array<T, N>::operator[]((std::size_t)e);
		}
	};
}