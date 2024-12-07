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

			void Signal() { Counter.fetch_sub(1); }

			void Add() { Counter.fetch_add(1); }

			bool IsNotCleared() { return Counter.load() > 0; }
		};

		void Initialize();
		void Finalize();
		void SubmitTask(std::function<void()> const& task);

		bool IsBusy();

		void Wait();

		void Wait(Barrier& barrier);

		void Signal(Barrier& barrier);

		size_t GetThreadCount();
	}

}

