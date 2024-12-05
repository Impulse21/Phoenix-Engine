#pragma once

#include <functional>
namespace phx
{
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
	}

}
