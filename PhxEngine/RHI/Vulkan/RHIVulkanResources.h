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

		// Per-image: has this swapchain image ever been transitioned out of
		// UNDEFINED yet? First use needs oldLayout=UNDEFINED, every use after
		// that needs oldLayout=PRESENT_SRC_KHR (where SubmitAndPresent leaves it).
		std::unique_ptr<bool[]>			vk_image_layout_initialized;

		u32			curr_sem_index = 0;
		u32 		curr_image_index = 0;
		u32 		image_count = 0;
		u32			width = 0;
		u32			height = 0;

		VkImage 	GetCurrentImage() 		{ return vk_images[curr_image_index]; }
		VkImageView GetCurrentImageView() 	{ return vk_image_views[curr_image_index];}
	};
    // static_assert(sizeof(ViewportImpl) <= PHX_CACHELINE, "Swapchain must fit within a cache line in size!");

	struct PHX_CACHELINE_ALIGN VulkanBuffer
	{
		// -- 8-byte members ---
		VkBuffer		vk_buffer;
		VmaAllocation	allocation;

		VkDeviceAddress gpu_address = 0;
		void* 			mapped_data = nullptr;
		VkBufferView	buffer_view = VK_NULL_HANDLE;

		// -- 4-byte members ---
		u32        		mapped_data_size = 0;
		
		std::byte _padding[20];
	};	
	static_assert(sizeof(VulkanBuffer) == PHX_CACHELINE, "VulkanBuffer must be exactly one cache line in size!");

	struct PHX_CACHELINE_ALIGN VulkanTexture
	{
		// -- 8-byte members ---
		VkImage			vk_image;
		VmaAllocation	allocation;

		VkImageView     vk_view_sampled = VK_NULL_HANDLE;
		VkImageView     vk_view_storage = VK_NULL_HANDLE;
		VkImageView     vk_view_rtv = VK_NULL_HANDLE;
		VkImageView     vk_view_dsv = VK_NULL_HANDLE;

		// -- 4-byte members ---
		VkImageLayout	default_layout = VK_IMAGE_LAYOUT_GENERAL;
		DescriptorIndex srv_index = kInvalidDescriptorIndex;
		DescriptorIndex uav_index = kInvalidDescriptorIndex;

		VkFormat vk_format = VK_FORMAT_UNDEFINED;

		uint32_t format_layout;

		// -- Extends passed cache line here ----
		// -- Dimentsion Could be packed into u32---
		u32 width;
		u32 height;

		// Has the one-time UNDEFINED -> GENERAL transition happened yet?
		// With unifiedImageLayouts, this is the only transition this image
		// ever needs — it stays in GENERAL for every usage after that.
		bool layout_initialized = false;
	};

	// Disable this for now as I am still working on things. Metadata should be stored into it's own pool of info.
	// static_assert(sizeof(VulkanTexture) == PHX_CACHELINE, "VulkanTexture must be exactly one cache line in size!");

	struct PHX_CACHELINE_ALIGN VulkanPipelineState
	{
		// -- 8-byte members ---
		VkPipeline                      vk_pipeline;
		VkPipelineBindPoint             bind_point;

		// -- 1-byte members ---
		bool                            graphics_pipeline = true;
	};
	static_assert(sizeof(VulkanPipelineState) == PHX_CACHELINE, "VulkanPipelineState must be exactly one cache line in size!");

	struct PHX_CACHELINE_ALIGN VulkanSampler
	{

	};
	static_assert(sizeof(VulkanSampler) == PHX_CACHELINE, "VulkanSampler must be exactly one cache line in size!");

	struct VulkanShaderModule
	{
		// -- 8-byte members ---
		VkShaderModule vk_shader_module = VK_NULL_HANDLE;
	};
	static_assert(sizeof(VulkanShaderModule) <= PHX_CACHELINE, "VulkanShaderModule must fit within a cache line in size!");
}
