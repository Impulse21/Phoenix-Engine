#pragma once

#include <PhxRhi/IGpuMemoryAllocator.h>
#include <vk_mem_alloc.h>

namespace phx::rhi
{
	struct VulkanDevice;
	class VulkanGpuMemoryAllocator final : public IGpuMemoryAllocator
	{
	public:
		VulkanGpuMemoryAllocator(VulkanDevice* vulkan_device);
		~VulkanGpuMemoryAllocator() override = default;
		VulkanGpuMemoryAllocator(const VulkanGpuMemoryAllocator&) = delete;	

		void Initialize();
		void Shutdown();
	public:
		Budget GetBudget() override;

	private:
		VulkanDevice* m_vulkan_device = nullptr;

		inline static VmaAllocator m_vma_allocator = VK_NULL_HANDLE;

		inline static phx::rhi::vk::GpuTempMemoryAllocator temp_memory_allocator;
	};
}

