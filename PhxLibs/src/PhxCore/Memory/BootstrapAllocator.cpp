#include <PhxCore/PhxCore_pch.h>

#include "BootstrapAllocator.h"
#include <PhxCore/Base.h>
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

			size_t aligned_start = AlignUp(g_allocated_size, alignment);
			if (aligned_start + size > kSize)
			{
				std::cerr << "CRITICAL ERROR: Bootstrap Memory Overflow\n";
				std::terminate();
				return nullptr;
			}

			g_allocated_size = aligned_start + size;

			return g_bootstrap_buffer + aligned_start;
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