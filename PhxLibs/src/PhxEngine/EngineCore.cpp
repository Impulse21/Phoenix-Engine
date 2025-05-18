#include "PhxEngine/PhxEngine_pch.h"
#include "EngineCore.h"

#include <PhxCore/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Memory.h>
#include <PhxCore/ThreadPool.h>
#include <PhxCore/CommandLineArgs.h>
#include <PhxCore/VFS.h>

#include <PhxRenderer/DefaultRenderSystem.h>

#include <PhxRhi/RHICore.h>

using namespace phx;

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
	}
}

namespace phx
{
	namespace EngineCore
	{
		void PreInitialize(int argc, wchar_t** argv)
		{
			phx::Log::Initialize();
			phx::CommandLineArgs::Initialize(argc, argv);
			phx::ThreadPool::Initialize();

			Memory::Initialize({ .VirtualMemorySize = 16_GiB });

			phx::IApplication::Ptr = phx::CreateApplication();
		}

		void Initialize(void* windowHandle)
		{
			auto* app = phx::IApplication::Ptr;
			uint32_t w, h;
			app->GetDefaultWindowSize(w, h);

			phx::rhi::Initialize({
				.SwapChianDesc = {.Width = w, .Height = h },
				.WindowsHandle = windowHandle
				});

			app->SetWindowHandle(windowHandle);

			phx::IRootFileSystem::Ptr = new RootFileSystem();
			phx::gfx::IRenderSystem::Ptr = new gfx::DefaultRenderSystem();

			app->Startup();
		}

		void Tick()
		{
			Memory::FrameAllocator& allocator = Memory::GetFrameAllocator();
			allocator.Reset();

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

			delete phx::IApplication::Ptr;
			phx::IApplication::Ptr = nullptr;

			delete gfx::IRenderSystem::Ptr;
			delete phx::IRootFileSystem::Ptr;

			phx::rhi::Finalize();
		}
	}
}