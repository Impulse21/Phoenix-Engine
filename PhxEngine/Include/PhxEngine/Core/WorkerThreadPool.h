#pragma once

#include <stdint.h>
#include <functional>

namespace PhxEngine::Core
{
	namespace WorkerThreadPool
	{
		using TaskID = uint64_t;
		struct TaskArgs
		{
			uint32_t JobIndex;		// job index relative to dispatch (like SV_DispatchThreadID in HLSL)
			uint32_t GroupID;		// group index relative to dispatch (like SV_GroupID in HLSL)
			uint32_t GroupIndex;	// job index relative to group (like SV_GroupIndex in HLSL)
			bool IsFirstJobInGroup;	// is the current job the first one in the group?
			bool IsLastJobInGroup;	// is the current job the last one in the group?
			void* SharedMemory = nullptr;
		};

		void Initialize(uint32_t maxThreadCount = ~0u);
		void Finalize();

		uint32_t GetThreadCount();

		TaskID Run(std::function<void(TaskArgs)>const& callback);
		TaskID Dispatch(uint32_t taskCount, uint32_t groupSize, std::function<void(TaskArgs)> const& callback);

		void WaitAll();
	}
}

