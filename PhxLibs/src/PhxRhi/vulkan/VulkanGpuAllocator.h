#pragma once

#include <PhxRhi/IGpuAllocator.h>
#include <vk_mem_alloc.h>

namespace phx::rhi
{
	struct VulkanBackend;
	struct VulkanGpuAllocator final : public IGpuMemoryAllocator
	{
		VulkanBackend* vulkan_backend = nullptr;
		inline static VmaAllocator vma_allocator = VK_NULL_HANDLE;

		bool Initialize() override;
		void Shutdown() override;

		Budget GetBudget() override;


		VulkanGpuAllocator(VulkanBackend* vulkan_device);

		~VulkanGpuAllocator() override = default;
		VulkanGpuAllocator(const VulkanGpuAllocator&) = delete;
	};
}

