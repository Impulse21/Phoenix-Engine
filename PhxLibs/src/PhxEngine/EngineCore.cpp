#include "PhxEngine/PhxEngine_pch.h"
#include "EngineCore.h"

#include <PhxCore/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Memory.h>
#include <PhxCore/ThreadPool.h>
#include <PhxCore/CommandLineArgs.h>
#include <PhxCore/VFS.h>
#include <PhxCore/Profiler.h>

#include <PhxResource/ResourceManger.h>

#include <PhxRenderer/DefaultRenderSystem.h>
#include <PhxRenderer/MeshResourceHandler.h>

#include <PhxRhi/GfxDevice.h>

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
	namespace EngineCore
	{
		size_t g_FrameCount = 0;
		void PreInitialize(int argc, wchar_t** argv)
		{
			g_FrameCount = 0;
			phx::Log::Initialize();
			phx::CommandLineArgs::Initialize(argc, argv);

			Memory::Initialize({
				.MaxMainHeapSize = 1_GiB });

			phx::ThreadPool::Initialize();

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

#if false
			app->SetWindowHandle(windowHandle);

			phx::IRootFileSystem::Ptr = phx_new_system(RootFileSystem);

			phx::ResourceManger::Initialize();
			phx::ResourceManger::RegisterHandler<renderer::MeshResourceHandler>();

			phx::gfx::IRenderSystem::Ptr = phx_new_system(gfx::DefaultRenderSystem);
#endif
			app->Startup();
		}

		void Tick()
		{
			PHX_PROFILE_FRAME;

			g_FrameCount++;

			// -- Pre-Render ---
			OnPreRender(phx::IApplication::Ptr);

			// -- Wait for any submitted taskes before finishing
			ThreadPool::Wait();

			// -- Update ---
			ThreadPool::SubmitTask([]() {
				OnUpdate_Threaded(phx::IApplication::Ptr, 0);
			});

			// -- Render ---
			ThreadPool::SubmitTask([]() {
				OnRender_Threaded(phx::IApplication::Ptr);
			});

			ThreadPool::Wait();
		}

		void Finalize()
		{
			phx::IApplication::Ptr->Shutdown();

			phx_delete_system(phx::IApplication::Ptr);
			phx::IApplication::Ptr = nullptr;

			phx::rhi::GfxDevice& gfxDevice = phx::rhi::GetDevice();
			gfxDevice.Shutdown();

#if false
			gfx::IRenderSystem::Ptr->Finalize();
			phx_delete_system(gfx::IRenderSystem::Ptr);

			phx_delete_system(phx::IRootFileSystem::Ptr);

			phx::rhi::Finalize();
#endif
			ThreadPool::Finalize();
			Memory::Shutdown();
		}
	}
}