#include "PhxEngine_pch.h"

#include <PhxEngine/EngineCore.h>

#include <PhxEngine/Application.h>

#include <PhxCore/Log.h>
#include <PhxCore/Profiler.h>
#include <PhxCore/VirtualFileSystem.h>
#include <PhxCore/SystemTime.h>
#include <PhxCore/JobSystem.h>
#include <PhxCore/Platform/PlatformWindow.h>
#include <PhxCore/Memory/FrameMemoryManager.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxResource/ResourceManager.h>
#include <PhxRenderer/MeshResourceHandler.h>
#include <PhxRenderer/MaterialResourceHandler.h>
#include <PhxRenderer/TextureResourceHandler.h>
#include <PhxResource/IO/IIoQueue.h>

#include <PhxWorld/PrefabResource.h>

#include <PhxEngine/EngineSync.h>

using namespace phx;

namespace
{
	phx::Window g_window = nullptr;
	std::unique_ptr<IApplication> g_application;

	// -- Owned Services ---
	std::unique_ptr<IIoQueue> g_io_queue;
	std::unique_ptr<IVirtualFileSystem> g_vfs;

	// -- Engine Info ---
	bool g_running = false;
	int64_t g_last_frame_time = 0;

	void InitializeServices(EngineServices engine_services)
	{
		if (!engine_services.virtual_file_system)
			engine_services.virtual_file_system = new VirtualFileSystem();
		
		g_vfs.reset(engine_services.virtual_file_system);
		IVirtualFileSystem::Ptr = g_vfs.get();

		if (!engine_services.io_queue)
			engine_services.io_queue = new phx::IoQueue();

		g_io_queue.reset(engine_services.io_queue);
		phx::IoQueue::Ptr = g_io_queue.get();
		{
			const bool use_dstorage = false;
			phx::IIoQueue::Ptr->Initialize(use_dstorage);
		}

		phx::ResourceManager::Initialize();
		phx::ResourceManager::RegisterLoader<renderer::MeshResourceHandler>(ResourceTraits<renderer::MeshResource>::Extension);
		phx::ResourceManager::RegisterLoader<renderer::TextureResourceHandler>(ResourceTraits<renderer::TextureResource>::Extension);
		phx::ResourceManager::RegisterLoader<renderer::MaterialResourceHandler>(ResourceTraits<renderer::MaterialResource>::Extension);
	}

	void LogCompiler()
	{
#ifdef __clang__
    PHX_CORE_INFO("Built with Clang version: {0}", __clang_version__);
#elif defined(__GNUC__) || defined(__GNUG__)
    PHX_CORE_INFO("Built with GCC");
#elif defined(_MSC_VER)
    PHX_CORE_INFO("Built with Microsoft Visual C++");
#else
    PHX_CORE_INFO("Built with an unknown compiler");
#endif
	}
}

namespace phx
{
	namespace EngineCore
	{
		static void Initialize(int argc, char ** argv);
		static void Finalize();
		static void Tick();

		void Run(int argc, char* argv[])
		{
			Initialize(argc, argv);
			
			while (g_running)
				Tick();

			Finalize();
		}

        void RequestExit()
        {
			g_running = false;
        }
		
        static void Initialize(int /*argc*/, char ** /*argv*/)
        {
			phx::Log::Initialize();
			PHX_CORE_INFO("Initializing Engine");

			LogCompiler();

			PHX_CORE_INFO("Initializing Core systems");
			{
				phx::JobSystem::Initialize();
				phx::FrameMemoryManager::Initialize({});
			}

			PHX_CORE_INFO("Create Application");
			g_application.reset(phx::CreateApplication());

			WindowDescriptor window_desc = {
				.width = 1,
				.height = 1,
				.title = g_application->GetName(),
				.FlagBits = 0
			};

			g_application->ConfigureWindow(window_desc);

			phx::Result<Window> window_result = Platform::CreateWindow(window_desc);
			if (!window_result)
			{
				PHX_CORE_ERROR("Failed to create window.");
				throw std::runtime_error("Failed to create window");
			}

			PHX_CORE_INFO("Create platform window [{0}, {1}]", window_desc.width, window_desc.height);
			g_window = *window_result;

			// -- Initializing RHI ---
			{
				const size_t thread_count = 
					JobSystem::GetThreadCount(JobSystem::Type::Generic) + 
					JobSystem::GetThreadCount(JobSystem::Type::Streaming);

				phx::window_native_handle native_handle = Platform::GetNativeHandle(g_window);
				phx::rhi::Initialize({}, &native_handle, thread_count);
			}

			EngineServices engine_services = {};
			g_application->ConfigureServices(engine_services);

			InitializeServices(engine_services);

			EngineContext engine_context ={
				.virtual_file_system = g_vfs.get(),
				.io_queue = g_io_queue.get(),
				.window = g_window,
			};
			
			g_application->Startup(engine_context);

			g_running = true;
			g_last_frame_time = phx::SystemTime::GetCurrentTick();
		}

		static void Finalize()
		{
			PHX_CORE_INFO("Shutting down Application");
			g_application->Shutdown();

			JobSystem::Wait();

			g_application.reset();

			phx::ResourceManager::Shutdown();

			g_io_queue->Shutdown();
			IIoQueue::Ptr = nullptr;
			g_io_queue.reset();

			g_vfs.reset();
			IVirtualFileSystem::Ptr = nullptr;

			rhi::Shutdown();

			JobSystem::Shutdown();

			FrameMemoryManager::ShutdownCurrentThreadFrameArena();
			FrameMemoryManager::Shutdown();
		}
		
		static void Tick()
		{
			if (!Platform::PollEvents(g_window))
			{
				PHX_CORE_INFO("Shutdown has been request. Shutting down.");
				g_running = false;
				return;
			}

			PHX_PROFILE_FRAME;

			const int64_t current_tick = phx::SystemTime::GetCurrentTick();
			
			float delta_time = std::min(0.1f, SystemTime::TicksToSeconds(current_tick - g_last_frame_time));

			g_last_frame_time = current_tick;

			EngineSync::g_frame_count++;
			JobSystem::Barrier sync;

			ThreadFrameArena* frame_allocator = FrameMemoryManager::GetCurrentThreadArenaPtr();

			// -- Pump IO Queue ---
			{
				auto* io_queue = IIoQueue::Ptr;
				io_queue->PollGpuCompletions();
				io_queue->SubmitBatchedWork(frame_allocator);
			}

			// -- Pre-Render ---
			sync.Add();
			JobSystem::SubmitJob([&sync](JobContext const& job_ctx) {
				g_application->OnPreRender(job_ctx.FrameHeap);
				sync.Signal();
			});

			// -- Sync point ---
			JobSystem::Wait(sync);

			// -- Update ---
			sync.Add();
			sync.Add();
			JobSystem::SubmitJob([&sync, delta_time](JobContext const& job_ctx) {
				g_application->OnUpdate_Threaded(delta_time, job_ctx.FrameHeap);
				sync.Signal();
			});

			// -- Render ---
			JobSystem::SubmitJob([&sync](JobContext const& job_ctx) {
				g_application->OnRender_Threaded(job_ctx.FrameHeap);
				sync.Signal();
			});

			JobSystem::Wait(sync);
		}
	}
}