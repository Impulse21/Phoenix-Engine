#include <PhxEngine/Core/WorkerThreadPool.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/Object.h>

#include <thread>
#include <deque>
#include <mutex>
#include <assert.h>

#ifdef WIN32
#define NOMINMAX  // Define NOMINMAX to prevent min and max macros
#include <Windows.h>
#endif
using namespace PhxEngine::Core;

// -- Internal ---
namespace
{
	struct Task
	{
		
	};

	class TaskQueue : public Object
	{
	private:
		std::deque<Task> m_taskQueue;
		std::mutex m_lock;
	};

	struct WorkerThreadPoolImpl
	{
		uint32_t NumCores = 0;
		uint32_t  NumThreads = 0;
		uint32_t MaxThreadCount = 0;

		TaskQueue* TaskQueues = nullptr;
		std::thread* Threads = nullptr;

		std::condition_variable WakeCondition;
		std::mutex WakeMutex;
		std::atomic_bool IsAlive;

		void Initialize(uint32_t maxThreadCount)
		{
			this->MaxThreadCount = std::max(1u, maxThreadCount);
			this->NumCores = std::thread::hardware_concurrency();
			this->NumThreads = std::min(this->MaxThreadCount, std::max(1u, this->NumCores - 1));

			// Allocate Array of Job Queues
			this->TaskQueues = phx_new_arr(TaskQueue, this->NumThreads);
			this->Threads = phx_new_arr(std::thread, this->NumThreads);

			for (size_t threadId = 0; threadId < this->NumThreads; threadId++)
			{
				this->Threads[threadId] = std::thread([this, threadId]() {
						while (this->IsAlive.load())
						{
							// TODDO work;
							PHX_LOG_CORE_INFO("Hello from thread ID [{0}]", threadId);

							std::unique_lock<std::mutex> lock(this->WakeMutex);
							this->WakeCondition.wait(lock);
						}
					});
#ifdef _WIN32
				// Do Windows-specific thread setup:
				HANDLE handle = (HANDLE)this->Threads[threadId].native_handle();

				// Put each thread on to dedicated core:
				DWORD_PTR affinityMask = 1ull << threadId;
				DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
				assert(affinity_result > 0);

				//// Increase thread priority:
				//BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
				//assert(priority_result != 0);

				// Name the thread:
				std::wstring wthreadname = L"PhxEngine::WorkerThreadPool::Worker[" + std::to_wstring(threadId) + L"i]";
				HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
				assert(SUCCEEDED(hr));
#else
				assert(false);
#endif
			}
		}

		bool IsInitialized() const { return this->Threads; }

		void Finalize()
		{
			if (!this->IsInitialized())
			{
				return;
			}

			this->IsAlive.store(false);
			bool runWakeThread = true;
			std::thread wakeThread([&]() {
					while (runWakeThread)
					{
						this->WakeCondition.notify_all();
					}
				});

			for (size_t i = 0; i < this->NumThreads; i++)
			{

				this->Threads[i].join();
			}

			runWakeThread = false;
			wakeThread.join();

			phx_delete_arr(this->Threads);
			phx_delete_arr(this->TaskQueues);

			this->Threads = nullptr;
			this->TaskQueues = nullptr;
		}

		~WorkerThreadPoolImpl()
		{
			this->Finalize();
		}

	} m_impl;
}


void Initialize(uint32_t maxThreadCount = ~0u)
{
	if (m_impl.IsInitialized() > 0u)
		return;

	m_impl.Initialize(maxThreadCount);


}
void Finalize()
{
	m_impl.Finalize();
}

WorkerThreadPool::TaskID WorkerThreadPool::Dispatch(uint32_t numTasks, uint32_t groupSize, std::function<void(WorkerThreadPool::TaskArgs)> const& callback)
{

}

void WorkerThreadPool::WaitAll()
{

}