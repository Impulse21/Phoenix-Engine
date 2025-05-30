#pragma once

#include <PhxRhi/BaseCommandCtx.h>
#include "VkGfxDevice.h"

#include <vulkan/vulkan.h>

namespace phx::rhi::vk
{
	class VkCommandCtxImpl : public BaseCommandBuffer<VkCommandCtxImpl>
	{
		// Friend BaseCommandBuffer to allow it to call Platform* methods
		friend class BaseCommandBuffer<VkCommandCtxImpl>;

	public:

		VkCommandCtxImpl() 
			: m_vkCommandBuffer(VK_NULL_HANDLE)
		{}

		void PlatfomrInitialize(VkCommandBuffer vkCommandBuffer, VkGfxDeviceImpl* device, uint32_t swapchainImageIndex)
		{
			m_vkCommandBuffer = vkCommandBuffer;
			m_swapchainImageIndex = swapchainImageIndex;
			m_gfxDevice = device;
		}

		VkCommandBuffer GetVkCommandBuffer() const
		{
			return m_vkCommandBuffer;
		}

	private:
		VkGfxDeviceImpl* m_gfxDevice;
		uint32_t m_swapchainImageIndex;
		VkCommandBuffer m_vkCommandBuffer;
	};

}
