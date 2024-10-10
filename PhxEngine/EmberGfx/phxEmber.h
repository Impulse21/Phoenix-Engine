#pragma once

#include "phxGfxDevice.h"
#include "phxPlatformDetection.h"

// -- Vulkan ---

#include "Vulkan/phxVulkanDevice.h"
namespace phx::gfx
{

	using GpuDevice = platform::VulkanGpuDevice;
	namespace EmberGfx
	{
		void Initialize(SwapChainDesc const& swapChainDesc, void* windowHandle);
		void Finalize();

		GpuDevice* GetDevice();
	}
}

