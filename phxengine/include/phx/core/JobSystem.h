#pragma once

#include <functional>
namespace phx
{
	struct JobDispatchArgs
	{
		uint32_t JobIndex = 0;
		uint32_t GroupIndex = 0;
	};

	
	namespace ThreadPool
	{
		struct Barrier
		{
			std::atomic_int Counter;
		};

		void Initialize();
		void Finalize();
		void Submit(std::function<void()> const& job);

		bool IsBusy();

		void Wait();

		void Wait(Barrier& barrier);

		void Signal(Barrier& barrier);

		size_t GetThreadCount();
	}

}

