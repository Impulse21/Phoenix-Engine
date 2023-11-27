#pragma once

#include <memory>
#include <vector>

#include <mimalloc-override.h>

namespace Phx
{
	template<typename T>
	using vector = std::vector<T, mi_stl_allocator<T>>;
}