#pragma once

#include "IAsyncIOSystem.h"
#include <deque>
#include <mutex>
#include <condition_variable>

namespace phx
{

	class StandardAsyncIOSystem final : public IAsyncIOSystem
	{
	public:
		bool Initialize() override;
		void Shutdown() override;

		void QueueRead(AsyncReadRequest&& request) override;

	private:
		void StreamingThreadLoop();
		void ProcessReadRequest(AsyncReadRequest& request);
	private:

		std::deque<AsyncReadRequest> m_requestQueue;
		std::mutex m_queueMutex;
		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;
	};
}

