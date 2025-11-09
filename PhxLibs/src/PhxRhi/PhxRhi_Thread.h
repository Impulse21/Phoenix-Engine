#pragma once

#include <cstdint>

namespace phx::rhi
{
	inline thread_local uint32_t g_rhi_thread_index = 0;
}