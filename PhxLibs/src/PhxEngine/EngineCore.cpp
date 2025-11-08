#include "PhxEngine/PhxEngine_pch.h"
#include "EngineCore.h"

#include <PhxCore/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Profiler.h>

#include <PhxResource/ResourceSystem.h>

#include <PhxRenderer/DefaultRenderSystem.h>
#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxCore/VirtualFileSystem.h>

#include <PhxReflection/Reflection.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxEngine/JobSystem.h>
#include <PhxEngine/EngineSync.h>
#include <PhxEngine/IStreamingManager.h>
#include <PhxEngine/Memory/FrameMemoryManager.h>
#include <PhxEngine/IO/IoQueue.h>

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

			phx::reflection::Initialize();

			phx::IVirtualFileSystem::Ptr = new VirtualFileSystem();

			phx::IApplication::Ptr = phx::CreateApplication();
		}

		void Initialize(void* window_handle)
		{
			auto* app = phx::IApplication::Ptr;
			uint32_t w, h;
			app->GetDefaultWindowSize(w, h);

			phx::rhi::Initialize({}, window_handle);

			app->SetSwapchain(g_swapchain, window_handle);

			phx::IIoQueue::Ptr = new phx::IoQueue();

			phx::ResourceSystem::Ptr = new ResourceSystem;
			phx::ResourceSystem::Ptr->Initialize(IVirtualFileSystem::Ptr);
			phx::ResourceSystem::Ptr->RegisterFileHanlder<renderer::MeshResourceHandler>();

#if false
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

			phx::ResourceSystem::Ptr->Shutdown();
			delete phx::ResourceSystem::Ptr;
			phx::ResourceSystem::Ptr = nullptr;

			IStreamingManager::Ptr->Shutdown();
			delete IStreamingManager::Ptr;

			delete IVirtualFileSystem::Ptr;
			IVirtualFileSystem::Ptr = nullptr;

			phx::reflection::Shutdown();

			rhi::IDevice::Ptr->DestroySwapchain(g_swapchain);
			rhi::Shutdown();

			JobSystem::Shutdown();

			FrameMemoryManager::ShutdownCurrentThreadFrameArena();
			FrameMemoryManager::Shutdown();
		}
	}
}