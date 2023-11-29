#pragma once

#include <vector>
#include <PhxEngine/Core/Memory.h>
#include <mimalloc.h>

namespace PhxEngine::Core
{
	template<typename T>
	using FlexArray = std::vector<T, mi_stl_allocator<T>>;
}