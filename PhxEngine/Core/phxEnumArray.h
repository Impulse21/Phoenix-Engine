#pragma once

#include <array>

namespace phx::core
{
	template<typename TEnum, class T, std::size_t N = (std::size_t)TEnum::Count>
	class EnumArray : public std::array<T, N>
	{
	public:
		T& operator[] (TEnum e)
		{
			return std::array<T, N>::operator[]((std::size_t)e);
		}

		const T& operator[] (TEnum e) const
		{
			return std::array<T, N>::operator[]((std::size_t)e);
		}

		T& operator[] (size_t i)
		{
			return std::array<T, N>::operator[](i);
		}

		const T& operator[] (size_t i) const
		{
			return std::array<T, N>::operator[](i);
		}
	};
}