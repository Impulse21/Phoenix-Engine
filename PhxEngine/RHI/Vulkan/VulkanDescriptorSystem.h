#pragma once

#include <PhxEngine/RHI/RHITypes.h>
#include "VulkanDescriptorHeap.h"

namespace phx::rhi::vulkan 
{
    struct DescriptorSystem 
    {
        static constexpr uint32_t max_resource_descriptors = 500'000;
        static constexpr uint32_t max_sampler_descriptors = 2048;
        // TODO: CHANGE THIS TO USE THE GLOBAL CONSTANT cMaxPushConstantSize
        static constexpr uint32_t max_push_constant_size = 256; // const uint32_t max_push_constant_size = 128;
       
        std::vector<VkSampler> global_samplers;

        DescriptorHeap resource_heap;
        DescriptorHeap sampler_heap;

        VkDescriptorSetLayout resource_layout;
        VkDescriptorSetLayout sampler_layout;
        VkPipelineLayout pipeline_layout;

        void Initialize(VkDevice vk_device, VmaAllocator vma_allocator, VkPhysicalDevice vk_physical_device);
        void Shutdown(VkDevice vk_device);

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
        void CreateMasterPipelineLayout(VkDevice vk_device);
        void CreateGlobalSamplers(VkDevice vk_device);
    };
}