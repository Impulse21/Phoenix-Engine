#pragma once

#include <stdint.h>

namespace PhxEngine::Core
{
	using TaskID = uint64_t;
	namespace WorkerThreadPool
	{

		void Initialize();
		void Finalize();

		uint32_t GetThreadCount();

		// void Run(context& ctx, const std::function<void(JobArgs)>& task);
	}
}

