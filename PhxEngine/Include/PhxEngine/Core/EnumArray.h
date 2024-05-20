#pragma once

#include <array>
<<<<<<< HEAD:PhxEngine/Core/EnumArray.h
namespace phx::rhi
=======
namespace PhxEngine
>>>>>>> c44c559ccb372caf73938f28953663abf1e0a85f:PhxEngine/Include/PhxEngine/Core/EnumArray.h
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