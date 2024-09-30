#pragma once
constexpr inline unsigned long long operator "" _KiB(unsigned long long value)
{
	return value << 10;
}

constexpr inline unsigned long long operator "" _MiB(unsigned long long value)
{
	return value << 20;
}

constexpr inline unsigned long long operator "" _GiB(unsigned long long value)
{
	return value << 30;
}
