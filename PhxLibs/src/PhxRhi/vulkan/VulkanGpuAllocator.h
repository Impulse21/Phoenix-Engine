#pragma once

#include <PhxRhi/RHICommon.h>
#include <vk_mem_alloc.h>

namespace phx::rhi
{
	struct VulkanBackend;
	struct VulkanGpuAllocator
	{
		VulkanBackend* vulkan_backend = nullptr;
		inline static VmaAllocator vma_allocator = VK_NULL_HANDLE;

		bool Initialize();
		void Shutdown();

		Budget GetBudget();


		VulkanGpuAllocator(VulkanBackend* vulkan_device);

		~VulkanGpuAllocator() = default;
		VulkanGpuAllocator(const VulkanGpuAllocator&) = delete;
	};
}

