#include <PhxCore/PhxCore_pch.h>

#include "BootstrapAllocator.h"
#include <iostream>

namespace
{
	constexpr size_t kSize = 256_KiB;
	std::mutex g_mutex;
	alignas(std::max_align_t) char g_bootstrap_buffer[kSize];
	char* g_next_ptr = g_bootstrap_buffer;
}

namespace phx
{
	namespace BootstrapAllocator
	{
		void* Allocate(size_t size, size_t alignment)
		{
			std::scoped_lock _(g_mutex);

			char* align_ptr = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(g_next_ptr) + (size_t)alignment - 1) & ~((size_t)alignment - 1));

			if (align_ptr + size > g_bootstrap_buffer + kSize)
			{
				std::cerr << "CRITICAL ERROR: Bootstrap Memory Overflow\n";
				std::terminate();
				return nullptr;
			}
			g_next_ptr = align_ptr + kSize;

			return g_next_ptr;
		}

		void* Allocate(size_t size, size_t alignment, const char*, int32_t)
		{
			return Allocate(size, alignment);
		}

		void Deallocate(void* /*pointer*/)
		{
			// no-op
		}
	}
}