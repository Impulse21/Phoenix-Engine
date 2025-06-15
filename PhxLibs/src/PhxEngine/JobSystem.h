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

		enum class Priority
		{
			High,
			Low,
			Count,
		};

		enum class Type
		{
			Generic = 0,
			Streaming,
			Count
		};

		void Initialize();
		void Shutdown();

		void SubmitJob(JobCallbackFunc const& task, Priority priority = Priority::High, JobContext* specifiedCtx = nullptr);
		void SubmitJobToStreaming(JobCallbackFunc const& task, JobContext* specifiedCtx = nullptr);

		bool IsBusy(Type type = Type::Generic);

		void Wait(Type type = Type::Generic);

		void Flush();

		void Wait(Barrier& barrier, Type type = Type::Generic);

		void Signal(Barrier& barrier, Type type = Type::Generic);

		uint32_t GetThreadCount(Type type = Type::Generic);
		uint32_t GetNumCores();
	}

}

