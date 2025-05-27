#pragma once

#include <functional>
#include <atomic>

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

		enum class Type
		{
			High = 0,
			Streaming,
			Count
		};

		void Initialize();
		void Finalize();
		void SubmitTask(std::function<void()> const& task, Type type = Type::High);

		bool IsBusy(Type type = Type::High);

		void Wait(Type type = Type::High);

		void Wait(Barrier& barrier, Type type = Type::High);

		void Signal(Barrier& barrier, Type type = Type::High);

		uint32_t GetThreadCount(Type type = Type::High);
		uint32_t GetNumCores();
	}

}

