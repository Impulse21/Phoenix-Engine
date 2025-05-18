#pragma once

#include "VFSResource.h"
#include <PhxCore/Pool.h>

#include <dstorage.h>
#include <deque>

#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>

namespace phx
{
	struct FileDS
	{
		Microsoft::WRL::ComPtr<IDStorageFile> DsFile;
		BY_HANDLE_FILE_INFORMATION FileInfo = {};
		Microsoft::WRL::ComPtr<IDStorageStatusArray> StatusArray;
	};

	// Ties together a Win32 event with a Windows Threadpool Wait.
	class EventWait
	{
		Microsoft::WRL::Wrappers::Event m_event;
		TP_WAIT* m_wait;

	public:
		EventWait(void* target, PTP_WAIT_CALLBACK callback)
		{
			m_wait = CreateThreadpoolWait(callback, target, nullptr);
			if (!m_wait)
				std::abort();

			constexpr BOOL manualReset = TRUE;
			constexpr BOOL initialState = FALSE;
			m_event.Attach(CreateEventW(nullptr, manualReset, initialState, nullptr));

			if (!m_event.IsValid())
				std::abort();
		}

		template<typename T, void (T::* FN)()>
		static EventWait Create(T* target)
		{
			auto callback = [](TP_CALLBACK_INSTANCE*, void* context, TP_WAIT*, TP_WAIT_RESULT)
				{
					T* target = reinterpret_cast<T*>(context);
					(target->*FN)();
				};

			return EventWait(target, callback);
		}

		~EventWait()
		{
			Close();
		}

		void SetThreadpoolWait()
		{
			ResetEvent(m_event.Get());
			::SetThreadpoolWait(m_wait, m_event.Get(), nullptr);
		}

		bool IsSet() const
		{
			return WaitForSingleObject(m_event.Get(), 0) == WAIT_OBJECT_0;
		}

		void Close()
		{
			if (m_wait)
			{
				WaitForThreadpoolWaitCallbacks(m_wait, TRUE);
				CloseThreadpoolWait(m_wait);
				m_wait = nullptr;
			}
		}

		operator HANDLE()
		{
			return m_event.Get();
		}
	};

	class CallbackQueue
	{
	public:
		CallbackQueue()
			: m_processCallbackEvent(EventWait::Create<CallbackQueue, &CallbackQueue::ProcessCallbacks>(this))
		{
		}

		void EnqueueCallback(IResourceFileSystem::RequestCallbackFunc&& callback)
		{
			std::lock_guard<std::mutex> lock(m_queueMutex);
			m_taskQueue.push_back(std::move(callback));
		}

		void SetThreadpoolWait()
		{
			m_processCallbackEvent.SetThreadpoolWait();
		}

		EventWait& GetEvent() { return m_processCallbackEvent; }
	private:
		void ProcessCallbacks()
		{
			// Swap task queues to avoid blocking enqueues
			{
				std::lock_guard<std::mutex> lock(m_queueMutex);
				std::swap(m_taskQueue, m_proccessingTaskQueue);
			}

			// Execute the tasks
			while (!m_proccessingTaskQueue.empty())
			{
				auto callback = std::move(m_proccessingTaskQueue.front());
				m_proccessingTaskQueue.pop_front();
				callback();
			}
		}
	private:
		EventWait m_processCallbackEvent;
		std::deque<IResourceFileSystem::RequestCallbackFunc> m_taskQueue;
		std::deque<IResourceFileSystem::RequestCallbackFunc> m_proccessingTaskQueue;
		std::mutex m_queueMutex;
	};

	class CallbackQueue;
	class DSResourceFileSystem final : public IResourceFileSystem
	{
	public:
		DSResourceFileSystem();
		~DSResourceFileSystem() override = default;

		FileHandle Open(std::filesystem::path const& path) override;
		void Close(FileHandle handle) override;

		void EnqueueRead(ReadRequest const& request) override;

		size_t GetFileSize(FileHandle handle) const override;
		void SubmitRequests(RequestCallbackFunc&& callback) override;

	public:
		void Mount(const std::filesystem::path& path, std::shared_ptr<IFileSystem> fs) override
		{
			m_rootFs->Mount(path, fs);
		}

		void Mount(const std::filesystem::path& path, const std::filesystem::path& nativePath) override
		{
			m_rootFs->Mount(path, nativePath);
		}

		bool Unmount(const std::filesystem::path& path) override
		{
			return m_rootFs->Unmount(path);
		}

		bool FileExists(std::filesystem::path const& name) override
		{
			return m_rootFs->FileExists(name);
		}

		bool FolderExists(std::filesystem::path const& name) override
		{
			return m_rootFs->FolderExists(name);
		}

		bool FolderCreate(std::filesystem::path const& name) override
		{
			return m_rootFs->FolderCreate(name);
		}

		std::unique_ptr<IBlob> ReadFile(std::filesystem::path const& name) override
		{
			return m_rootFs->ReadFile(name);
		}

		bool WriteFile(std::filesystem::path const& name, Span<char> Data) override
		{
			return m_rootFs->WriteFile(name, Data);
		}

		std::filesystem::path ResolvePath(std::filesystem::path const& name) override
		{
			return m_rootFs->ResolvePath(name);
		}
		
	private:
		std::unique_ptr<IRootFileSystem> m_rootFs;
		CallbackQueue m_callbackQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue1> m_dsSystemMemoryQueue;

		phx::PagedPool<File, FileDS> m_filePool;
	};
}

