#pragma once

#include <memory>
#include <vector>

#include <mimalloc.h>

namespace Phx
{
	template<typename T>
	using FlexArray = std::vector<T, mi_stl_allocator<T>>;
}