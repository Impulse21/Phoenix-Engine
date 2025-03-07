#include <Phoenix.h>
#include <mutex>
#include <atomic>

#include "PhxCore/EntryPoint.h"
#include "PhxCore/StringHash.h"
#include "PhxCore/VFS.h"
#include "Generated/GlobalVariables.h"

#include "PhxRenderer/ImGuiRenderer.h"

#include "PhxRenderer/MeshResourceHandler.h"
#include "PhxResource/ResourceManger.h"

#include <PhxCore/ThreadPool.h>

#define TEST_TREAD_POOL 0
#if TEST_TREAD_POOL 
#include <chrono>
#include <thread>
#endif

using namespace phx;

namespace
{
#if TEST_TREAD_POOL
	void ThreadPoolTest()
	{
		ThreadPool::Initialize();
		ThreadPool::SubmitTask([] {

			for (int i = 0; i < 20; i++)
			{
				PHX_INFO("Streaming A");
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
			}, ThreadPool::Type::Streaming);

		ThreadPool::SubmitTask([]() {

			ThreadPool::Barrier barrier;
			PHX_INFO("Tasking A");
			barrier.Add();
			ThreadPool::SubmitTask([&]() {
				for (int i = 0; i < 10; i++)
				{
					PHX_INFO("Tasking B");
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				ThreadPool::Signal(barrier);
				});

			ThreadPool::SubmitTask([&]() {
				for (int i = 0; i < 10; i++)
				{
					PHX_INFO("Tasking C");
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
				ThreadPool::Signal(barrier);
				});

			PHX_INFO("Waiting on Tasks A");
			ThreadPool::Wait(barrier);

			PHX_INFO("Finished waiting on Tasks A");

			});

		PHX_INFO("---->Wainting for all task");
		ThreadPool::Wait();

		PHX_INFO("---->Waiting for streaming");
		ThreadPool::Wait(ThreadPool::Type::Streaming);
		PHX_INFO("---->Waint is done");
	}
#endif
}
class Sandbox final : public phx::IApplication
{
public:
	static Sandbox* Instance() { return ms_instance; }

public:
	Sandbox(const phx::ApplicationDescriptor& desc)
		: m_desc(desc)
	{
		ms_instance = this;
	}

	~Sandbox() { ms_instance = nullptr; }

	void Startup() override
	{
		PHX_INFO("Sandbox app is starting up");

#if TEST_TREAD_POOL
		ThreadPoolTest();
#endif
		m_imguiRenderer.Initialize(m_fs.get(), m_windowHandle);
		m_imguiRenderer.EnableDarkThemeColours();

		m_loadingBarrier.Add();
		ThreadPool::SubmitTask([this] {

			m_fs = phx::FileSystemFactory::CreateRootFileSystem();

			phx::ResourceManger::Initialize(GlobalPaths::AssetsDirectory);
			phx::ResourceManger::RegisterHandler<phx::renderer::MeshResourceHandler>();
			phx::ResourceManger::RegisterPakFile("res:/NewSponza_Main_glTF_003.phxpak");

			ThreadPool::Wait(ThreadPool::Type::Streaming);

			m_meshResource = phx::ResourceManger::Get("res:/NewSponza_Main_glTF_003/lionhead.phxmsh");

			m_loadingBarrier.Signal();
			PHX_INFO("--->Decremented value {0}", m_loadingBarrier.Counter.load());
		});
	}

	void Shutdown() override
	{
		PHX_INFO("Sandbox app is starting up");
		m_imguiRenderer.Finialize();

		ThreadPool::Finalize();
	}

	void Tick() override
	{
		m_imguiRenderer.BeginFrame();

		if (m_loadingBarrier.IsNotCleared())
		{
			uint32_t width, height;
			GetDefaultWindowSize(width, height);
			ImGui::GetForegroundDrawList()->AddText(ImVec2(width / 2, height / 2), IM_COL32(255, 255, 255, 255), "Loading");
		}
		else
		{
			phx::ResourceManger::DrawGui();
		}

		rhi::CommandCtx* ctx = rhi::BeginCommnadCtx();
		ctx->RenderPassBegin();
		m_imguiRenderer.Render(ctx);
		ctx->RenderPassEnd();

		phx::rhi::Present();
	}

	const char* GetName() const override { return this->m_desc.Name.c_str(); }
	void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const override
	{
		outWidth = m_desc.Width;
		outHeight = m_desc.Height;
	}

	void SetWindowHandle(void* handle) override { m_windowHandle = handle; }
	void* GetWindowHandle() const override { return m_windowHandle; }

private:
	inline static Sandbox* ms_instance = nullptr;

	
private:
	ThreadPool::Barrier m_loadingBarrier;
	std::unique_ptr<phx::IRootFileSystem> m_fs;
	const phx::ApplicationDescriptor m_desc;
	phx::gfx::ImGuiRenderSystem m_imguiRenderer;

	RefCountPtr<IResource> m_meshResource;
	void* m_windowHandle;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "Sandbox",
		.WorkingDirectory = phx::VFS::GetDirectoryWithExecutable()
	};

	return new Sandbox(desc);
}
