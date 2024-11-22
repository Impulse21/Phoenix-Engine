#pragma once

// Ripped from Wicked engine: https://wickedengine.net/2018/11/simple-job-system-using-standard-c/comment-page-1/
#include <functional>
namespace phx
{
	struct JobDispatchArgs
	{
		uint32_t JobIndex = 0;
		uint32_t GroupIndex = 0;
	};

	namespace JobSystem
	{
		void Initialize();
		void Execute(std::function<void()> const& job);

		void Dispatch(uint32_t jobCount, uint32_t groupSize, std::function<void(JobDispatchArgs)> const& job);

		bool IsBusy();

		void Wait();
	}
}

