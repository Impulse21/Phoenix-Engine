#pragma once

#include <PhxData/IStreamingManager.h>
#include <PhxData/IVirtualFileSystem.h>
#include <PhxCore/Platform/PlatformWrapper.h>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace phx
{
	namespace rhi
	{
		class GfxDevice;
	}
}
namespace phx
{
	class StandardStreamingManager final : public data::IStreamingManager
	{
	public:
		StandardStreamingManager(data::IVirtualFileSystem* vfs)
			: m_vfs(vfs)
		{
		}

		void Initialize() override;
		void Shutdown() override;

		void Submit(data::StreamingRequest&& request) override;

		void Tick(float delta_time) override;

	public:
		platform::PlatformFileHandle FindOrCreateHandle(std::string const& file_path);

	private:
		void StreamingThreadLoop();

	private:
		data::IVirtualFileSystem* m_vfs = nullptr;

		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;

		std::deque<data::StreamingRequest> m_requestQueue;
		std::mutex m_queueMutex;

		std::unordered_map<std::string, platform::PlatformFileHandle> m_fileHandleCache;
		std::mutex m_fileHandleCacheMutex;
	};
}

