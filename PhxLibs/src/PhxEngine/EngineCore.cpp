#include "PhxEngine/PhxEngine_pch.h"
#include "EngineCore.h"

#include <PhxCore/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Memory.h>
#include <PhxCore/CommandLineArgs.h>
#include <PhxCore/VFS.h>
#include <PhxCore/Profiler.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/DefaultRenderSystem.h>
#include <PhxRenderer/MeshResourceHandler.h>

#include <PhxRhi/GfxDevice.h>

#include <PhxEngine/JobSystem.h>
#include <PhxEngine/EngineSync.h>

using namespace phx;

#ifdef PHX_PLATFORM_WINDOWS
HWND g_hWnd;
HINSTANCE g_hInstance;
#endif
namespace
{
	void OnPreRender(IApplication* app)
	{
		app->OnPreRender();
	}

	void OnUpdate_Threaded(IApplication* app, float deltaTime)
	{
		app->OnUpdate_Threaded(deltaTime);
	}

	void OnRender_Threaded(IApplication* app)
	{
		app->OnRender_Threaded();
		//phx::rhi::Present();
	}
}

namespace phx
{
	namespace EngineSync
	{
		size_t g_FrameCount = 0;
	}

	namespace EngineCore
	{
		void PreInitialize(int argc, wchar_t** argv)
		{
			EngineSync::g_FrameCount = 0;
			phx::Log::Initialize();
			phx::CommandLineArgs::Initialize(argc, argv);

			Memory::Initialize({
				.MaxMainHeapSize = 1_GiB });

			phx::JobSystem::Initialize();

			phx::IApplication::Ptr = phx::CreateApplication();
		}

		void Initialize(void* windowHandle)
		{
			auto* app = phx::IApplication::Ptr;
			uint32_t w, h;
			app->GetDefaultWindowSize(w, h);

			phx::rhi::GfxDevice& gfxDevice = phx::rhi::GetDevice();
			gfxDevice.Initialize({
				.SwapChainDesc = {.Width = w, .Height = h },
				.WindowsHandle = windowHandle,
				.MaxNumTextures = 1000,
				.MaxNumGpuBuffers = 1000,
				.MaxNumPipelineStates = 1000
				});

			app->SetWindowHandle(windowHandle);

			phx::IRootFileSystem::Ptr = phx_new_persistent(RootFileSystem);
			phx::ResourceSystem::Ptr = phx_new_persistent(ResourceSystem);
			phx::ResourceSystem::Ptr->Initialize(phx::IRootFileSystem::Ptr);
#if false

			phx::ResourceManger::Initialize();
			phx::ResourceManger::RegisterHandler<renderer::MeshResourceHandler>();

			phx::gfx::IRenderSystem::Ptr = phx_new_system(gfx::DefaultRenderSystem);
#endif
			app->Startup();
		}

		void Tick()
		{
			PHX_PROFILE_FRAME;

			EngineSync::g_FrameCount++;

			// -- Pre-Render ---
			OnPreRender(phx::IApplication::Ptr);

			// -- Wait for any submitted taskes before finishing
			JobSystem::Wait();

			// -- Update ---
			JobSystem::SubmitJob([](JobContext const&) {
				OnUpdate_Threaded(phx::IApplication::Ptr, 0);
			});

			// -- Render ---
			JobSystem::SubmitJob([](JobContext const&) {
				OnRender_Threaded(phx::IApplication::Ptr);
			});

			JobSystem::Wait();
		}

		void Finalize()
		{
			phx::IApplication::Ptr->Shutdown();

			DeleteApplication(phx::IApplication::Ptr);
			phx::IApplication::Ptr = nullptr;

			phx::ResourceSystem::Ptr->Shutdown();
			phx_delete_persistent(phx::ResourceSystem::Ptr);
			phx::ResourceSystem::Ptr = nullptr;

			phx::rhi::GfxDevice& gfxDevice = phx::rhi::GetDevice();
			gfxDevice.Shutdown();

			phx_delete_persistent(phx::IRootFileSystem::Ptr);
			phx::IRootFileSystem::Ptr = nullptr;


#if false
			gfx::IRenderSystem::Ptr->Finalize();
			phx_new_persistent(gfx::IRenderSystem::Ptr);

			phx::rhi::Finalize();
#endif
			JobSystem::Shutdown();
			Memory::Shutdown();
		}
	}
}