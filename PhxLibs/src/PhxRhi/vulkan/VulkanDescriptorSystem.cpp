#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanDescriptorSystem.h"

#include "VulkanBackend.h"
#include "VulkanTypes.h"

using namespace phx;
using namespace phx::rhi;

void phx::rhi::vulkan::DescriptorSystem::Initialize(VulkanBackend* vulkan_backend)
{
    resource_heap.Initialize(vulkan_backend, vulkan::HeapType::Resource, max_resource_descriptors);
    sampler_heap.Initialize(vulkan_backend, vulkan::HeapType::Sampler, max_sampler_descriptors);

    CreateMasterPipelineLayout(vulkan_backend);


}

void phx::rhi::vulkan::DescriptorSystem::Bind(VkCommandBuffer cmd)
{
    VkDescriptorBufferBindingInfoEXT resource_bindings[4] = {};
    VkDeviceAddress resource_addr = resource_heap.GetBufferAddress();

    for (int i = 0; i < 4; ++i) 
    {
        resource_bindings[i].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
        resource_bindings[i].address = resource_addr;
        resource_bindings[i].usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    // 2. Prepare Sampler Heap Binding Info
    // "For Set 1, Binding 0, use THAT address."
    VkDescriptorBufferBindingInfoEXT sampler_binding = {};
    sampler_binding.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    sampler_binding.address = m_sampler_heap.GetBufferAddress();
    sampler_binding.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;

    // 3. Bind the Buffers
    // NOTE: The indices here must match the "Global Binding Index"
    // Since Set 0 has 4 bindings, Set 1's binding 0 is actually index 4 ? 
    // NO: vkCmdBindDescriptorBuffersEXT works on a straight array of bindings index-by-index.
    // We need to bind them carefully.

    // Bind Set 0 (Indices 0, 1, 2, 3)
    vkCmdBindDescriptorBuffersEXT(cmd, 4, resource_bindings);

    // Bind Set 1 (Index 0) - Wait, we need to map sets to buffer indices.
    // We need `vkCmdSetDescriptorBufferOffsetsEXT` to map Sets -> Buffer Indices.

    // Let's configure the offsets to make this robust:
    // Pipeline Layout tells Vulkan:
    // Set 0: Binding 0, 1, 2, 3
    // Set 1: Binding 0

    // We are assigning:
    // Buffer Index 0 -> Set 0, Binding 0
    // Buffer Index 1 -> Set 0, Binding 1
    // Buffer Index 2 -> Set 0, Binding 2
    // Buffer Index 3 -> Set 0, Binding 3
    // Buffer Index 4 -> Set 1, Binding 0

    // Bind the sampler info as the 5th buffer (index 4)
    vkCmdBindDescriptorBuffersEXT(cmd, 1, &sampler_binding); // This appends? No.

    // Correct approach: Bind ALL buffers in one call or use offsets.
    VkDescriptorBufferBindingInfoEXT all_bindings[5] = {
        resource_bindings[0],
        resource_bindings[1],
        resource_bindings[2],
        resource_bindings[3],
        sampler_binding
    };
    vkCmdBindDescriptorBuffersEXT(cmd, 5, all_bindings);

    // 4. Set Offsets
    // We need to tell the pipeline: "Set 0 starts at buffer index 0, Set 1 starts at buffer index 4"
    // Also the offset INTO the buffer is always 0 for us (start of heap).

    uint32_t buffer_indices_set0[] = { 0, 1, 2, 3 };
    VkDeviceSize offsets_set0[] = { 0, 0, 0, 0 };

    uint32_t buffer_indices_set1[] = { 4 };
    VkDeviceSize offsets_set1[] = { 0 };

    // Apply offsets for Set 0
    vkCmdSetDescriptorBufferOffsetsEXT(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
        0, // firstSet
        4, // bindingCount (0,1,2,3)
        buffer_indices_set0,
        offsets_set0);

    // Apply offsets for Set 1
    vkCmdSetDescriptorBufferOffsetsEXT(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout,
        1, // firstSet
        1, // bindingCount (0)
        buffer_indices_set1,
        offsets_set1);
}

void phx::rhi::vulkan::DescriptorSystem::CreateMasterPipelineLayout(VulkanBackend* vulkan_backend)
{
    {
        // Define the bindings for Textures, Buffers, Images, etc.
        VkDescriptorSetLayoutBinding resource_bindings[] = {
            { // -- Textures ---
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .descriptorCount = max_resource_descriptors,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            },
            { // -- Buffers (UNIFORM_BUFFER) ---
                .binding = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = max_resource_descriptors,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            },
            { // -- Buffers (STORAGE_BUFFER) ---
                .binding = 2,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .descriptorCount = max_resource_descriptors,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            },
            { // -- RW Images (STORAGE_IMAGE) ---
                .binding = 3,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = max_resource_descriptors,
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            },
        };

        VkDescriptorBindingFlags bindless_flags =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT; // Optional for descriptor_buffer, but good practice

        std::vector<VkDescriptorBindingFlags> binding_flags(std::size(resource_bindings), bindless_flags);

        VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = (uint32_t)binding_flags.size(),
            .pBindingFlags = binding_flags.data(),
        };

        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &binding_flags_info,
            .bindingCount = (uint32_t)std::size(resource_bindings),
            .pBindings = resource_bindings,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
        };

        vulkan_check(
            vkCreateDescriptorSetLayout(vulkan_backend->vk_device, &layout_info, nullptr, &resource_layout));
    }

    {
        VkDescriptorSetLayoutBinding sampler_binding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = max_sampler_descriptors,
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr,
        };

        VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = 1,
            .pBindingFlags = &bindless_flags,
        };

        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &binding_flags_info,
            .bindingCount = 1,
            .pBindings = &sampler_binding,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
        };

        vulkan_check(
            vkCreateDescriptorSetLayout(vulkan_backend->vk_device, &layout_info, nullptr, &sampler_layout));
    }

    VkDescriptorSetLayout sets[] = { resource_layout, sampler_layout };

    // 2. Define MAX push constant range (e.g. 128 bytes)
    VkPushConstantRange push_constant = {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = max_push_constant_size,
    };

    // 3. Bake the Master Layout
    VkPipelineLayoutCreateInfo layout_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 2,
        .pSetLayouts = sets,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant,
    };

    vkCreatePipelineLayout(vulkan_backend->vk_device, &layout_ci, nullptr, &pipeline_layout);
}

void phx::rhi::vulkan::DescriptorSystem::Shutdown(VulkanBackend* vulkan_backend)
{
    if (pipeline_layout == VK_NULL_HANDLE) 
        vkDestroyPipelineLayout(vulkan_backend->vk_device, pipeline_layout, nullptr);

    if (resource_layout == VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(vulkan_backend->vk_device, resource_layout, nullptr);

    if (sampler_layout == VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(vulkan_backend->vk_device, sampler_layout, nullptr);

    resource_heap.Shutdown();
    sampler_heap.Shutdown();
}
