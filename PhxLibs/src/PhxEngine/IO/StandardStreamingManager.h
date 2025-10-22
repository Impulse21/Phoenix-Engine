#pragma once

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxCore/Platform/PlatformWrapper.h>

#include <PhxEngine/IStreamingManager.h>

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
	class StandardStreamingManager final : public IStreamingManager
	{
	public:
		StandardStreamingManager(IVirtualFileSystem* vfs)
			: m_vfs(vfs)
		{
		}

		void Initialize() override;
		void Shutdown() override;

		void Submit(StreamingRequest&& request) override;

		void Tick(float delta_time) override;

	public:
		platform::PlatformFileHandle FindOrCreateHandle(std::string const& file_path);

	private:
		void StreamingThreadLoop();

	private:
		IVirtualFileSystem* m_vfs = nullptr;

		std::condition_variable m_cv;
		std::atomic<bool> m_shutdown;

		std::deque<StreamingRequest> m_requestQueue;
		std::mutex m_queueMutex;

		std::unordered_map<std::string, platform::PlatformFileHandle> m_fileHandleCache;
		std::mutex m_fileHandleCacheMutex;
	};
}

