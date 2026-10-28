#include "Jobs.h"

#include <PhxEngine/Core/CVar.h>

#if defined(PHX_PROFILING_ENABLED)
    #include <tracy/Tracy.hpp>
#endif

#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

PHX_CVAR_INT(jobs_thread_count, 0, "Jobs worker thread count (0 = hardware_concurrency)");

namespace
{
    bool m_is_alive = false;
    u32 m_num_threads = 0;
}

void phx::Jobs::Initialize(u32 thread_count)
{
	const uint32_t num_cores = std::thread::hardware_concurrency();
    m_num_threads
    
}
