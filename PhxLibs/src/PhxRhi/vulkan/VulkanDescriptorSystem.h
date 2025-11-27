#pragma once
#include <vulkan/vulkan.h>

#include "VulkanDescriptorHeap.h"

namespace phx::rhi
{
    struct VulkanBackend;
}

namespace phx::rhi::vulkan 
{

    struct DescriptorSystem 
    {
        const uint32_t max_resource_descriptors = 500'000;
        const uint32_t max_sampler_descriptors = 2048;
        const uint32_t max_push_constant_size = 128;

        DescriptorHeap resource_heap;
        DescriptorHeap sampler_heap;

        VkDescriptorSetLayout resource_layout;
        VkDescriptorSetLayout sampler_layout;
        VkPipelineLayout pipeline_layout;

        void Initialize(VulkanBackend* vulkan_backend);
        void Shutdown(VulkanBackend* vulkan_backend);

        rhi::DescriptorIndex AllocateResource(const VkDescriptorGetInfoEXT& info)
        {
            return resource_heap.Allocate(info);
        }
        rhi::DescriptorIndex AllocateSampler(const VkDescriptorGetInfoEXT& info)
        {
            return sampler_heap.Allocate(info);
        }

        void FreeResource(rhi::DescriptorIndex index)
        {
            resource_heap.Free(index);
        }

        void FreeSampler(rhi::DescriptorIndex index) 
        {
            sampler_heap.Free(index);
        }

        void Bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point);

    private:
        void CreateMasterPipelineLayout(VulkanBackend* vulkan_backend);
    };
}