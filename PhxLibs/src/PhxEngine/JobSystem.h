#pragma once

#include <functional>
#include <atomic>

namespace phx
{
	class IAllocator;

	struct JobDispatchArgs
	{
		uint32_t JobIndex = 0;
		uint32_t GroupIndex = 0;
	};
	struct JobContext
	{
		IAllocator* FrameHeap = nullptr;
	};

	static_assert(sizeof(JobContext) <= 16, "Try to keep this small as we do a copy of this data");

	namespace JobSystem
	{
		using JobCallbackFunc = std::function<void(JobContext const&)>;
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
		void Shutdown();

		void SubmitJob(JobCallbackFunc const& task, Type type = Type::High, JobContext* specifiedCtx = nullptr);

		bool IsBusy(Type type = Type::High);

		void Wait(Type type = Type::High);

		void Wait(Barrier& barrier, Type type = Type::High);

		void Signal(Barrier& barrier, Type type = Type::High);

		uint32_t GetThreadCount(Type type = Type::High);
		uint32_t GetNumCores();
	}

}

