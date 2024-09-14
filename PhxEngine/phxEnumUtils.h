#pragma once

#include <array>

// Defines all bitwise operators for enum classes so it can be (mostly) used as a regular flags enum
#define PHX_ENUM_CLASS_FLAGS(Enum) \
	inline           Enum& operator|=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
	inline           Enum& operator&=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
	inline           Enum& operator^=(Enum& Lhs, Enum Rhs) { return Lhs = (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator| (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs | (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator& (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs & (__underlying_type(Enum))Rhs); } \
	inline constexpr Enum  operator^ (Enum  Lhs, Enum Rhs) { return (Enum)((__underlying_type(Enum))Lhs ^ (__underlying_type(Enum))Rhs); } \
	inline constexpr bool  operator! (Enum  E)             { return !(__underlying_type(Enum))E; } \
	inline constexpr Enum  operator~ (Enum  E)             { return (Enum)~(__underlying_type(Enum))E; }



namespace phx
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

	template<typename Enum>
	constexpr bool EnumHasAllFlags(Enum Flags, Enum Contains)
	{
		using UnderlyingType = __underlying_type(Enum);
		return ((UnderlyingType)Flags & (UnderlyingType)Contains) == (UnderlyingType)Contains;
	}

	template<typename Enum>
	constexpr bool EnumHasAnyFlags(Enum Flags, Enum Contains)
	{
		using UnderlyingType = __underlying_type(Enum);
		return ((UnderlyingType)Flags & (UnderlyingType)Contains) != 0;
	}

	template<typename Enum>
	void EnumAddFlags(Enum& Flags, Enum FlagsToAdd)
	{
		using UnderlyingType = __underlying_type(Enum);
		Flags = (Enum)((UnderlyingType)Flags | (UnderlyingType)FlagsToAdd);
	}

	template<typename Enum>
	void EnumRemoveFlags(Enum& Flags, Enum FlagsToRemove)
	{
		using UnderlyingType = __underlying_type(Enum);
		Flags = (Enum)((UnderlyingType)Flags & ~(UnderlyingType)FlagsToRemove);
	}
}