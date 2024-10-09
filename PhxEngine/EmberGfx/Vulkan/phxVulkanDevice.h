#pragma once

#include "EmberGfx/phxGfxDeviceResources.h"

namespace phx::gfx::platform
{
	class VulkanDevice
	{
	public:
		static void Initialize(SwapChainDesc const& swapChainDesc, bool enableValidationLayers, void* windowHandle = nullptr);
		static void Finalize();

	private:
		static void CreateInstance();

	private:
		static inline VkInstance m_vkInstance;
	};
}

