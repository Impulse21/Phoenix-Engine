#pragma once

#include <PhxCore/Handle.h>
#include <PhxCore/Platform/Platform.h>

#include <functional>
#include <atomic>

namespace phx
{
	struct ThreadPool;
	using ThreadPoolHandle = Handle<ThreadPool>;

    struct ThreadPoolDescriptor
    {
        std::string name;
        uint32_t num_threads = 1;   // 0 = use (hardware_concurrency - 1)
        Platform::ThreadPriority os_priority = Platform::ThreadPriority::Normal;
        bool has_low_queue = false; // whether this pool services a Low-priority queue
    };

	namespace TaskScheduler
	{
		using TaskCallbackFunc = std::function<void()>;
		struct Barrier
		{
			std::atomic_int Counter = 0;

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


		void Initialize();
		void Shutdown();

		ThreadPoolHandle CreateThreadPool(const ThreadPoolDescriptor& desc);

		ThreadPoolHandle GetCorePool();
		ThreadPoolHandle InitializeCorePool();

		void Submit(const TaskCallbackFunc& task, ThreadPoolHandle pool_handle, Priority priority = Priority::High);
		
		bool IsBusy(ThreadPoolHandle pool_handle);

		void Wait(ThreadPoolHandle pool_handle);
		void Wait(Barrier& barrier, ThreadPoolHandle pool_handle);
		void Signal(Barrier& barrier, ThreadPoolHandle pool_handle);

		void Flush();

		uint32_t GetThreadCount(ThreadPoolHandle pool_handle);
		uint32_t GetNumCores();
	}

}

