#include "PhxEngine/PhxEngine_pch.h"
#include "EngineCore.h"

#include <PhxCore/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Profiler.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/DefaultRenderSystem.h>
#include <PhxRenderer/MeshResourceHandler.h>

#include <PhxData/VirtualFileSystemImpl.h>
#include <PhxData/AssetManager.h>

#include <PhxRhi/GfxDevice.h>

#include <PhxEngine/JobSystem.h>
#include <PhxEngine/EngineSync.h>
#include <PhxEngine/Memory/FrameMemoryManager.h>
#include <PhxEngine/IO/StandardAsyncIOSystem.h>

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
		void PreInitialize(int /*argc*/, wchar_t** /*argv*/)
		{
			EngineSync::g_FrameCount = 0;
			phx::Log::Initialize();

			FrameMemoryManager::Initialize({});
			
			phx::JobSystem::Initialize();

			phx::data::IVirtualFileSystem::Ptr = new data::VirtualFileSystemImpl();

			phx::data::IAsyncIOSystem::Ptr = new phx::StandardAsyncIOSystem(phx::data::IVirtualFileSystem::Ptr);
			phx::data::IAsyncIOSystem::Ptr->Initialize();

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

			phx::ResourceSystem::Ptr = new ResourceSystem;
			phx::ResourceSystem::Ptr->Initialize(phx::data::IVirtualFileSystem::Ptr, phx::data::IAsyncIOSystem::Ptr);

			phx::data::AssetManager::Ptr = new phx::data::AssetManager;
			phx::data::AssetManager::Ptr->Initialize(phx::data::IVirtualFileSystem::Ptr, phx::data::IAsyncIOSystem::Ptr);

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
			JobSystem::SubmitJob([](JobContext const&) {
				OnPreRender(phx::IApplication::Ptr);
				JobSystem::Wait();
			});

			JobSystem::Barrier sync;

			// -- Update ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const&) {
				OnUpdate_Threaded(phx::IApplication::Ptr, 0);
				JobSystem::Wait();
				sync.Signal();
			});

			// -- Render ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const&) {
				OnRender_Threaded(phx::IApplication::Ptr);
				JobSystem::Wait();
				sync.Signal();
			});

			JobSystem::Wait(sync);
		}

		void Finalize()
		{
			phx::IApplication::Ptr->Shutdown();

			DeleteApplication(phx::IApplication::Ptr);
			phx::IApplication::Ptr = nullptr;

			phx::data::AssetManager::Ptr->Shutdown();
			delete phx::data::AssetManager::Ptr;
			phx::data::AssetManager::Ptr = nullptr;

			phx::ResourceSystem::Ptr->Shutdown();
			delete phx::ResourceSystem::Ptr;
			phx::ResourceSystem::Ptr = nullptr;

			phx::rhi::GfxDevice& gfxDevice = phx::rhi::GetDevice();
			gfxDevice.Shutdown();

			phx::data::IAsyncIOSystem::Ptr->Shutdown();
			delete phx::data::IAsyncIOSystem::Ptr;

			delete phx::data::IVirtualFileSystem::Ptr;
			phx::data::IVirtualFileSystem::Ptr = nullptr;

			JobSystem::Shutdown();

			FrameMemoryManager::ShutdownCurrentThreadFrameArena();
			FrameMemoryManager::Shutdown();
		}
	}
}