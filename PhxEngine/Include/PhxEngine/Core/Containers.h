#pragma once

#include <vector>
#include <PhxEngine/Core/Memory.h>

namespace PhxEngine::Core
{

	template<typename T, typename A = PhxStlAllocator<T>>
	using FlexArray = std::vector<T, A>;
}