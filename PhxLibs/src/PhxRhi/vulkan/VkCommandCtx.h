#pragma once

#include "VkCommon.h"
#include <PhxRhi/BaseCommandCtx.h>


namespace phx::rhi::vk
{
	class VkGfxCommandCtx : public BaseGfxCommnadCtx<VkGfxCommandCtx>
	{
		// Friend BaseCommandBuffer to allow it to call Platform* methods
		friend class BaseGfxCommnadCtx<VkGfxCommandCtx>;

	public:

		VkGfxCommandCtx()
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

	class VkComputeCommandCtx : public BaseComputeCommnadCtx<VkComputeCommandCtx>
	{
		// Friend BaseCommandBuffer to allow it to call Platform* methods
		friend class BaseGfxCommnadCtx<VkComputeCommandCtx>;

	public:

		VkComputeCommandCtx()
			: m_vkCommandBuffer(VK_NULL_HANDLE)
		{
		}

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

	class VkCopyCommandCtx : public BaseCopyCommnadCtx<VkCopyCommandCtx>
	{
		// Friend BaseCommandBuffer to allow it to call Platform* methods
		friend class BaseCopyCommnadCtx<VkCopyCommandCtx>;

	public:

		VkCopyCommandCtx()
			: m_vkCommandBuffer(VK_NULL_HANDLE)
		{
		}

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
