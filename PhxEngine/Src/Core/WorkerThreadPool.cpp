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
		uint32_t GroupId = 0;
		uint32_t GroupJobOffset;
		uint32_t GroupJobEnd;
		uint32_t SharedMemorySize = 0;
		std::function<void(WorkerThreadPool::TaskArgs)> Callback;

	};

	class TaskQueue : public Object
	{
	public:
		bool PopFront(Task& outTask)
		{
			std::scoped_lock lock(this->m_lock);
			if (this->m_taskQueue.empty())
			{
				return false;
			}
			outTask = std::move(this->m_taskQueue.front());
			this->m_taskQueue.pop_front();
			return true;
		}

		void PushBack(Task const& task)
		{
			std::scoped_lock lock(this->m_lock);
			m_taskQueue.push_back(task);
		}

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
							this->Work(threadId);

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

		void Work(size_t startingQueue)
		{
			for (size_t i = 0; i < this->NumThreads; i++)
			{
				TaskQueue& queue = this->TaskQueues[startingQueue % this->MaxThreadCount];

				Task task = {};
				while (queue.PopFront(task))
				{
					WorkerThreadPool::TaskArgs taskArgs = {};
					taskArgs.GroupID = task.GroupId;
					if (task.SharedMemorySize)
					{
						assert(false);
					}
					else
					{
						taskArgs.SharedMemory = nullptr;
					}


					for (uint32_t j = task.GroupJobOffset; j < task.GroupJobEnd; ++j)
					{
						taskArgs.JobIndex = j;
						taskArgs.GroupIndex = j - task.GroupJobOffset;
						taskArgs.IsFirstJobInGroup = (j == task.GroupJobOffset);
						taskArgs.IsLastJobInGroup = (j == task.GroupJobEnd - 1);
						task.Callback(taskArgs);
					}

					// TODO Wait for task to be done.
					// job.ctx->counter.fetch_sub(1);
				}

				// Move to next queue to steal work
				startingQueue++;
			}
		}

		uint32_t DispatchGroupCount(uint32_t taskCount, uint32_t groupSize)
		{
			// Calculate the amount of job groups to dispatch (overestimate, or "ceil"):
			return (taskCount + groupSize - 1) / groupSize;
		}

		~WorkerThreadPoolImpl()
		{
			this->Finalize();
		}

	} m_impl;

	std::atomic_size_t NextQueue;
}


void PhxEngine::Core::WorkerThreadPool::Initialize(uint32_t maxThreadCount)
{
	if (m_impl.IsInitialized() > 0u)
		return;

	m_impl.Initialize(maxThreadCount);


}
void PhxEngine::Core::WorkerThreadPool::Finalize()
{
	m_impl.Finalize();
}

WorkerThreadPool::TaskID PhxEngine::Core::WorkerThreadPool::Dispatch(std::function<void(TaskArgs)> const& callback)
{
	Task task;
	task.Callback = callback;
	task.GroupId = 0;
	task.GroupJobOffset = 0;
	task.GroupJobEnd = 1;
	task.SharedMemorySize = 0;

	m_impl.TaskQueues[NextQueue.fetch_add(1) % m_impl.NumThreads].PushBack(task);
	m_impl.WakeCondition.notify_one();
	return 0;
}

WorkerThreadPool::TaskID WorkerThreadPool::Dispatch(uint32_t taskCount, uint32_t groupSize, std::function<void(WorkerThreadPool::TaskArgs)> const& callback)
{
	if (taskCount == 0 || groupSize == 0)
	{
		return ~0u;
	}

	const uint32_t groupCount = m_impl.DispatchGroupCount(taskCount, groupSize);

	// Context state is updated:
	// ctx.counter.fetch_add(groupCount);

	Task task;
	task.Callback = callback;
	task.SharedMemorySize = (uint32_t)0;

	for (uint32_t groupID = 0; groupID < groupCount; ++groupID)
	{
		// For each group, generate one real job:
		task.GroupId = groupID;
		task.GroupJobOffset = groupID * groupSize;
		task.GroupJobEnd = std::min(task.GroupJobOffset + groupSize, taskCount);

		m_impl.TaskQueues[NextQueue.fetch_add(1) % m_impl.NumThreads].PushBack(task);
	}

	m_impl.WakeCondition.notify_all();

	return 0;
}

void WorkerThreadPool::WaitAll()
{
	// TODO
}

uint32_t WorkerThreadPool::GetThreadCount()
{
	return m_impl.NumThreads;
}