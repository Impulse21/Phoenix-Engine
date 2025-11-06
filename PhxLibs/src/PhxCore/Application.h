#pragma once

#include <string>
#include <filesystem>

#include <PhxRhi/RHICommon.h>
namespace phx
{
	struct ApplicationDescriptor
	{
		std::string Name = "Phoenix Application";
		std::filesystem::path WorkingDirectory = "";
		uint32_t Width = 1600;
		uint32_t Height = 900;
	};

	class IApplication
	{
	public:
		inline static IApplication* Ptr = nullptr;

	public:
		virtual ~IApplication() = default;

		virtual void OnPreRender() = 0;
		virtual void OnUpdate_Threaded(float deltaTime) = 0;
		virtual void OnRender_Threaded() = 0;

		virtual void Startup() = 0;
		virtual void Shutdown() = 0;

		virtual void SetSwapchain(rhi::SwapchainHandle swapchain, void* handle) = 0;
		virtual void* GetWindowHandle() const = 0;
		virtual const char* GetName() const = 0;
		virtual void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const = 0;
	};

	// To be defined in CLIENT

	IApplication* CreateApplication();
}

