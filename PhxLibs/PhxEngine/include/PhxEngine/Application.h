#pragma once

#include <string>
#include <filesystem>

#include <PhxCore/Memory/IAllocator.h>
#include <PhxCore/Platform/PlatformWindow.h>

#include <PhxRhi/PhxRhi.h>

#include <PhxEngine/EngineServices.h>
#include <PhxEngine/EngineContext.h>

namespace phx
{
	class IApplication
	{
	public:
		virtual ~IApplication() = default;

		virtual void ConfigureServices(EngineServices& /*services*/) {};
		virtual void ConfigureWindow(WindowDescriptor& /*win_desc*/) {};

		virtual void Startup(const EngineContext& engine_context) = 0;
		virtual void Shutdown() = 0;

		virtual void OnPreRender(IAllocator* frame_allocator) = 0;
		virtual void OnUpdate_Threaded(float deltaTime, IAllocator* frame_allocator) = 0;
		virtual void OnRender_Threaded(IAllocator* frame_allocator) = 0;

		virtual const char* GetName() const = 0;
	};

	// To be defined in CLIENT

	IApplication* CreateApplication();
}

