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

#include <PhxReflection/Reflection.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxEngine/JobSystem.h>
#include <PhxEngine/EngineSync.h>
#include <PhxEngine/Memory/FrameMemoryManager.h>
#include <PhxEngine/IO/StandardStreamingManager.h>

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
		//phx::RHI::Present();
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

			phx::reflection::Initialize();

			phx::data::IVirtualFileSystem::Ptr = new data::VirtualFileSystemImpl();

			phx::IApplication::Ptr = phx::CreateApplication();
		}

		void Initialize(void* windowHandle)
		{
			auto* app = phx::IApplication::Ptr;
			uint32_t w, h;
			app->GetDefaultWindowSize(w, h);

			RHI::Initialize({
				.SwapChainDesc = {.Width = w, .Height = h },
				.WindowsHandle = windowHandle,
				.MaxNumTextures = 1000,
				.MaxNumGpuBuffers = 1000,
				.MaxNumPipelineStates = 1000
				});

			app->SetWindowHandle(windowHandle);

			phx::data::IStreamingManager::Ptr = new phx::StandardStreamingManager(phx::data::IVirtualFileSystem::Ptr);
			phx::data::IStreamingManager::Ptr->Initialize();

			phx::ResourceSystem::Ptr = new ResourceSystem;
			phx::ResourceSystem::Ptr->Initialize(phx::data::IVirtualFileSystem::Ptr, phx::data::IStreamingManager::Ptr);

			phx::data::AssetManager::Ptr = new phx::data::AssetManager;
			phx::data::AssetManager::Ptr->Initialize(phx::data::IVirtualFileSystem::Ptr, phx::data::IStreamingManager::Ptr);

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

			JobSystem::Barrier sync;

			// -- Pre-Render ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const&) {
				OnPreRender(phx::IApplication::Ptr);
				sync.Signal();
			});

			JobSystem::Wait(sync);

			// -- Update ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const&) {
				OnUpdate_Threaded(phx::IApplication::Ptr, 0);
				sync.Signal();
			});

			// -- Render ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const&) {
				OnRender_Threaded(phx::IApplication::Ptr);
				sync.Signal();
			});

			JobSystem::Wait(sync);
		}

		void Finalize()
		{
			PHX_CORE_INFO("Shutting down Application");
			phx::IApplication::Ptr->Shutdown();

			JobSystem::Wait();

			DeleteApplication(phx::IApplication::Ptr);
			phx::IApplication::Ptr = nullptr;

			phx::data::AssetManager::Ptr->Shutdown();
			delete phx::data::AssetManager::Ptr;
			phx::data::AssetManager::Ptr = nullptr;

			phx::ResourceSystem::Ptr->Shutdown();
			delete phx::ResourceSystem::Ptr;
			phx::ResourceSystem::Ptr = nullptr;

			phx::data::IStreamingManager::Ptr->Shutdown();
			delete phx::data::IStreamingManager::Ptr;

			delete phx::data::IVirtualFileSystem::Ptr;
			phx::data::IVirtualFileSystem::Ptr = nullptr;

			phx::reflection::Shutdown();

			RHI::Shutdown();

			JobSystem::Shutdown();

			FrameMemoryManager::ShutdownCurrentThreadFrameArena();
			FrameMemoryManager::Shutdown();
		}
	}
}