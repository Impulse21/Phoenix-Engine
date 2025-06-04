#pragma once

#include <PhxData/IAsyncIOSystem.h>
#include <PhxData/IVirtualFileSystem.h>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace phx
{

	class StandardAsyncIOSystem final : public data::IAsyncIOSystem
	{
	public:
		StandardAsyncIOSystem(data::IVirtualFileSystem* vfs)
			: m_vfs(vfs)
		{
		}

		void Initialize() override;
		void Shutdown() override;

		void QueueRead(data::AsyncReadRequest&& request) override;

		void Tick(float delta_time) override;

	private:
		void StreamingThreadLoop();
		void ProcessReadRequest(data::AsyncReadRequest& request);

	private:
		data::IVirtualFileSystem* m_vfs = nullptr;
		std::deque<data::AsyncReadRequest> m_requestQueue;
		std::mutex m_queueMutex;
		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;
	};
}

