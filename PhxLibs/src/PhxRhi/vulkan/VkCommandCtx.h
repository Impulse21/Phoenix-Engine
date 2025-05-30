#pragma once

#include <PhxRhi/BaseCommandCtx.h>
#include <vulkan/vulkan.h>

namespace phx::rhi::vk
{
	struct VkCommandCtxImpl : public BaseCommandBuffer<VkCommandCtxImpl>
	{	
		// Friend BaseCommandBuffer to allow it to call Platform* methods
		friend class BaseCommandBuffer<VkCommandCtxImpl>;

		VkCommandCtxImpl() 
			: vk_command_buffer(VK_NULL_HANDLE) {}


		VkCommandBuffer vk_command_buffer;
	};

}
