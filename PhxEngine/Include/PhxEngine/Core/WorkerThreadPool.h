#pragma once

#include <stdint.h>

namespace PhxEngine::Core
{
	using TaskID = uint64_t;
	namespace WorkerThreadPool
	{
		struct TaskArgs
		{

		};

		void Initialize(uint32_t maxThreadCount = ~0u);
		void Finalize();

		uint32_t GetThreadCount();

		// void Run(context& ctx, const std::function<void(JobArgs)>& task);
		TaskID Dispatch(uint32_t numTasks, uint32_t groupSize, std::function<void(TaskArgs)>& callback);
	}
}

