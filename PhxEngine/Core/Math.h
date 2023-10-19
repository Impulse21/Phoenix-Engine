#pragma once

#include <stdint.h>

namespace PhxEngine::Core::Math
{

	constexpr uint64_t GetNextPowerOfTwo(uint64_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32u;
		return ++x;
	}
}