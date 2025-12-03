#include "PhxEngine/PhxEngine_pch.h"
#include "EngineCore.h"

#include <PhxCore/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/VirtualFileSystem.h>
#include <PhxCore/SystemTime.h>

#include <PhxResource/ResourceSystem.h>
#include <PhxRenderer/DefaultRenderSystem.h>
#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxRenderer/MaterialResourceHandler.h>
#include <PhxRenderer/TextureResourceHandler.h>

#include <PhxReflection/Reflection.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxEngine/JobSystem.h>
#include <PhxEngine/EngineSync.h>
#include <PhxEngine/Memory/FrameMemoryManager.h>
#include <PhxEngine/IO/IoQueue.h>

using namespace phx;

#ifdef PHX_PLATFORM_WINDOWS
HWND g_hWnd;
HINSTANCE g_hInstance;
#endif

namespace
{
	std::unique_ptr<IIoQueue> g_io_queue;
	int32_t g_last_frame_time = 0;

	void OnPreRender(IApplication* app, IAllocator* frame_allocator)
	{
		app->OnPreRender(frame_allocator);
	}

	void OnUpdate_Threaded(IApplication* app, float deltaTime, IAllocator* frame_allocator)
	{
		app->OnUpdate_Threaded(deltaTime, frame_allocator);
	}

	void OnRender_Threaded(IApplication* app, IAllocator* frame_allocator)
	{
		app->OnRender_Threaded(frame_allocator);
	}
}

namespace phx
{
	namespace EngineCore
	{
		void PreInitialize(int /*argc*/, wchar_t** /*argv*/)
		{
			phx::Log::Initialize();

			FrameMemoryManager::Initialize({});
			
			phx::JobSystem::Initialize();

			phx::reflection::Initialize();

			phx::IVirtualFileSystem::Ptr = new VirtualFileSystem();

			phx::IApplication::Ptr = phx::CreateApplication();
		}

		void Initialize(void* window_handle)
		{
			PHX_CORE_INFO("Initializing Engine");

			auto* app = phx::IApplication::Ptr;
			uint32_t w, h;
			app->GetDefaultWindowSize(w, h);

			const size_t thread_count = 
				JobSystem::GetThreadCount(JobSystem::Type::Generic) + 
				JobSystem::GetThreadCount(JobSystem::Type::Streaming);

			phx::rhi::Initialize({}, window_handle, thread_count);

			app->SetWindowHandle(window_handle);

			phx::IIoQueue::Ptr = new phx::IoQueue();
			g_io_queue.reset(phx::IIoQueue::Ptr);

			const bool use_dstorage = false;
			phx::IIoQueue::Ptr->Initialize(use_dstorage);

			phx::ResourceSystem::Ptr = new ResourceSystem;
			phx::ResourceSystem::Ptr->Initialize(IVirtualFileSystem::Ptr);
			phx::ResourceSystem::Ptr->RegisterFileHanlder<renderer::MeshResourceHandler>();
			phx::ResourceSystem::Ptr->RegisterFileHanlder<renderer::MaterialResourceHandler>();
			phx::ResourceSystem::Ptr->RegisterFileHanlder<renderer::TextureResourceHandler>();

	
#if false
			phx::gfx::IRenderSystem::Ptr = phx_new_system(gfx::DefaultRenderSystem);
#endif
			g_last_frame_time = phx::SystemTime::GetCurrentTick();
			app->Startup();
		}

		void Tick()
		{
			PHX_PROFILE_FRAME;

			const int64_t current_tick = phx::SystemTime::GetCurrentTick();
			
			float delta_time = std::min(0.1f, static_cast<float>(current_tick - g_last_frame_time));

			g_last_frame_time = current_tick;

			EngineSync::g_frame_count++;
			JobSystem::Barrier sync;

			ThreadFrameArena* frame_allocator = FrameMemoryManager::GetCurrentThreadArenaPtr();

			// -- Pump IO Queue ---
			{
				// Consider threading this
				// the risk is that we might miss things that are loaded already.
				auto* io_queue = IIoQueue::Ptr;

				io_queue->PollGpuCompletions();
				io_queue->SubmitBatchedWork(frame_allocator);
			}

			// -- Pre-Render ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const& job_ctx) {
				OnPreRender(phx::IApplication::Ptr, job_ctx.FrameHeap);
				sync.Signal();
			});

			// -- Sync point ---
			JobSystem::Wait(sync);

			// -- Update ---
			sync.Add();
			sync.Add();
			JobSystem::SubmitJob([&sync, delta_time](JobContext const& job_ctx) {
				OnUpdate_Threaded(phx::IApplication::Ptr, delta_time, job_ctx.FrameHeap);
				sync.Signal();
			});

			// -- Render ---
			JobSystem::SubmitJob([&sync](JobContext const& job_ctx) {
				OnRender_Threaded(phx::IApplication::Ptr, job_ctx.FrameHeap);
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

			IIoQueue::Ptr->Shutdown();
			g_io_queue.reset();

			delete IVirtualFileSystem::Ptr;
			IVirtualFileSystem::Ptr = nullptr;

			phx::reflection::Shutdown();
			rhi::Shutdown();

			JobSystem::Shutdown();

			FrameMemoryManager::ShutdownCurrentThreadFrameArena();
			FrameMemoryManager::Shutdown();
		}
	}
}