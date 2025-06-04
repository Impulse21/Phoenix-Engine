#pragma once

#include <cstdint>

namespace phx
{
	namespace BootstrapAllocator
	{

		[[nodiscard]] void* Allocate(size_t size, size_t alignment);
		[[nodiscard]] void* Allocate(size_t size, size_t alignment, const char* , int32_t );

		void Deallocate(void* pointer);
	}
}