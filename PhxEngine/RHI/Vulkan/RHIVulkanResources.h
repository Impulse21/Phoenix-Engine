#pragma once


#include <volk.h>
#include <vk_mem_alloc.h>

#include <memory>

namespace phx::rhi::vulkan
{
	struct ViewportImpl
	{
		// -- 8-byte members ---
		std::unique_ptr<VkSemaphore[]>	vk_image_available_sem;
		std::unique_ptr<VkSemaphore[]>	vk_render_finished_sem;

		std::unique_ptr<VkImage[]>		vk_images;
		std::unique_ptr<VkImageView[]>	vk_image_views;

        VkSurfaceKHR                    vk_surface      = VK_NULL_HANDLE;
		VkSwapchainKHR				    vk_swapchain    = VK_NULL_HANDLE;
		VkFormat					    vk_swapchain_image_format = VK_FORMAT_UNDEFINED;

		u32			curr_sem_index = 0;
		u32 		curr_image_index = 0;
		u32 		image_count = 0;
		u32			width = 0;
		u32			height = 0;

		VkImage 	GetCurrentImage() 		{ return vk_images[curr_image_index]; }
		VkImageView GetCurrentImageView() 	{ return vk_image_views[curr_image_index];}
	};
    // static_assert(sizeof(ViewportImpl) <= PHX_CACHELINE, "Swapchain must fit within a cache line in size!");

	struct CommandBufferImpl
	{
		rhi::CommandQueueType queue_type;
		VkCommandBuffer cmd_buffer 	= VK_NULL_HANDLE;
	};
	static_assert(sizeof(CommandBufferImpl) <= PHX_CACHELINE, "Swapchain must fit within a cache line in size!");
}
