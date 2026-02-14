#include "PhxRhi_pch.h"
#include "VulkanDescriptorSystem.h"


using namespace phx;
using namespace phx::rhi;

namespace
{
    // Helper to create a VkSamplerCreateInfo cleanly
    VkSamplerCreateInfo MakeSamplerInfo(
        VkFilter filter,
        VkSamplerAddressMode address_mode,
        bool enable_anisotropy = false,
        bool enable_compare = false)
    {
        VkSamplerCreateInfo info = { 
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = filter,
            .minFilter = filter,
            .mipmapMode = (filter == VK_FILTER_LINEAR) ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST,
            .addressModeU = address_mode,
            .addressModeV = address_mode,
            .addressModeW = address_mode,
            .mipLodBias = 0.0f,
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
        };

        if (enable_anisotropy) 
        {
            info.anisotropyEnable = VK_TRUE;
            info.maxAnisotropy = 16.0f;
        }
        else 
        {
            info.maxAnisotropy = 1.0f;
        }

        if (enable_compare) 
        {
            info.compareEnable = VK_TRUE;
            info.compareOp = VK_COMPARE_OP_LESS; // Standard shadow map comparison
        }

        return info;
    }

}
void phx::rhi::vulkan::DescriptorSystem::Initialize(
    VkDevice vk_device,
    VmaAllocator vma_allocator,
    VkPhysicalDevice vk_physical_device)
{
    PHX_RHI_INFO("Initializing Descriptor System system");


    PHX_RHI_INFO("Initializing resource heap of {0} descriptors", max_resource_descriptors);
    resource_heap.Initialize(vk_device, vma_allocator, vk_physical_device, vulkan::HeapType::Resource, max_resource_descriptors);

    PHX_RHI_INFO("Initializing sampler heap of {0} descriptors", max_sampler_descriptors);
    sampler_heap.Initialize(vk_device, vma_allocator, vk_physical_device, vulkan::HeapType::Sampler, max_sampler_descriptors);

    CreateGlobalSamplers(vk_device);
    CreateMasterPipelineLayout(vk_device);
}

void phx::rhi::vulkan::DescriptorSystem::Bind(VkCommandBuffer cmd, VkPipelineBindPoint bind_point)
{
    VkDeviceAddress res_addr = resource_heap.GetBufferAddress();
    VkDeviceAddress samp_addr = sampler_heap.GetBufferAddress();
    VkDescriptorBufferBindingInfoEXT bindings[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .pNext = nullptr,
            .address = res_addr,
            .usage = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
        },
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
            .pNext = nullptr,
            .address = samp_addr,
            .usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT
        },
    };

    vkCmdBindDescriptorBuffersEXT(cmd, 2, bindings);

    uint32_t set0_index = 0;
    VkDeviceSize set0_offset = 0;
    vkCmdSetDescriptorBufferOffsetsEXT(cmd, bind_point, pipeline_layout, 0, 1, &set0_index, &set0_offset);

    // 3. Map SET 1 -> Buffer 1
    uint32_t set1_index = 1;
    VkDeviceSize set1_offset = 0;
    vkCmdSetDescriptorBufferOffsetsEXT(cmd, bind_point, pipeline_layout, 1, 1, &set1_index, &set1_offset);
}

void phx::rhi::vulkan::DescriptorSystem::CreateMasterPipelineLayout(VkDevice vk_device)
{

    /*
     * Default behavior assigns binding indices based on descriptor types:
     *
     * Enum Value                  Vulkan Descriptor Type                       Binding Index
     * --------------------------------------------------------------------------------------
     * Sampler                     VK_DESCRIPTOR_TYPE_SAMPLER                   0
     * CombinedTextureSampler      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER    1
     * Texture_Read                VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE             2
     * Texture_ReadWrite           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE             2
     * TexelBuffer_Read            VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER      2
     * TexelBuffer_ReadWrite       VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER      2
     * Buffer_Read                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER            2
     * Buffer_ReadWrite            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER            2
     */
    VkDescriptorType mutable_types[] = {
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    };

    VkMutableDescriptorTypeListEXT type_list = {};
    type_list.descriptorTypeCount = 2;
    type_list.pDescriptorTypes = mutable_types;

    VkMutableDescriptorTypeCreateInfoEXT mutable_info = {
        .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
        .mutableDescriptorTypeListCount = 1,
        .pMutableDescriptorTypeLists = &type_list,
    };

	{
		// Define the bindings for Textures, Buffers, Images, etc.
		VkDescriptorSetLayoutBinding resource_binding = {
				.binding = 0,
				.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT,
				.descriptorCount = max_resource_descriptors,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = nullptr,
		};

        VkDescriptorBindingFlags bindless_flags =
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .pNext = &mutable_info,
            .bindingCount = 1,
            .pBindingFlags = &bindless_flags,
        };

        VkDescriptorSetLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &binding_flags_info,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
            .bindingCount = 1,
            .pBindings = &resource_binding,
        };

        vulkan_check(
            vkCreateDescriptorSetLayout(vk_device, &layout_info, nullptr, &resource_layout));
    }
    {
        VkDescriptorSetLayoutBinding sampler_binding = {
            0, VK_DESCRIPTOR_TYPE_SAMPLER, max_sampler_descriptors, VK_SHADER_STAGE_ALL, nullptr
        };

        VkDescriptorBindingFlags samp_flag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        VkDescriptorSetLayoutBindingFlagsCreateInfo samp_flags_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = 1,
            .pBindingFlags = &samp_flag,
        };

        VkDescriptorSetLayoutCreateInfo samp_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &samp_flags_info,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
            .bindingCount = 1,
            .pBindings = &sampler_binding,
        };

        vulkan_check(
            vkCreateDescriptorSetLayout(vk_device, &samp_info, nullptr, &sampler_layout));
    }

    // =========================================================================
    // PIPELINE LAYOUT
    // =========================================================================
    VkDescriptorSetLayout sets[] = { resource_layout, sampler_layout };

    PHX_RHI_INFO("Max push constant size per shader is ", max_push_constant_size);
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

    vkCreatePipelineLayout(vk_device, &layout_ci, nullptr, &pipeline_layout);
}

void phx::rhi::vulkan::DescriptorSystem::CreateGlobalSamplers(VkDevice vk_device)
{
    struct SamplerDefinition 
    {
        VkFilter filter;
        VkSamplerAddressMode address;
        bool aniso;
        bool compare;
    };

    // Order matters! This defines g_samplers[0], [1], etc.
    SamplerDefinition definitions[] = {
        { .filter = VK_FILTER_LINEAR,  .address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,    .aniso = false, .compare = false }, // 0: Linear Clamp
        { .filter = VK_FILTER_LINEAR,  .address = VK_SAMPLER_ADDRESS_MODE_REPEAT,           .aniso = false, .compare = false }, // 1: Linear Wrap
        { .filter = VK_FILTER_NEAREST, .address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,    .aniso = false, .compare = false }, // 2: Point Clamp
        { .filter = VK_FILTER_NEAREST, .address = VK_SAMPLER_ADDRESS_MODE_REPEAT,           .aniso = false, .compare = false }, // 3: Point Wrap
        { .filter = VK_FILTER_LINEAR,  .address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,    .aniso = true,  .compare = false }, // 4: Aniso Clamp
        { .filter = VK_FILTER_LINEAR,  .address = VK_SAMPLER_ADDRESS_MODE_REPEAT,           .aniso = true,  .compare = false }, // 5: Aniso Wrap
        { .filter = VK_FILTER_LINEAR,  .address = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,  .aniso = false, .compare = true  }  // 6: Shadow PCF
    };

    for (const auto& def : definitions)
    {
        VkSamplerCreateInfo info = MakeSamplerInfo(def.filter, def.address, def.aniso, def.compare);

        if (def.compare) 
        {
            info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        }

        VkSampler sampler;
        vkCreateSampler(vk_device, &info, nullptr, &sampler);
        global_samplers.push_back(sampler);

        VkDescriptorGetInfoEXT descriptor_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .data = {.pSampler = &sampler },
        };

        sampler_heap.Allocate(descriptor_info);
    }
}

void phx::rhi::vulkan::DescriptorSystem::Shutdown(VkDevice vk_device)
{
    if (pipeline_layout == VK_NULL_HANDLE) 
        vkDestroyPipelineLayout(vk_device, pipeline_layout, nullptr);

    if (resource_layout == VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(vk_device, resource_layout, nullptr);

    if (sampler_layout == VK_NULL_HANDLE)
        vkDestroyDescriptorSetLayout(vk_device, sampler_layout, nullptr);

    for (size_t i = 0; i <  global_samplers.size(); ++i) 
    {
		sampler_heap.Free((rhi::DescriptorIndex)i);
        vkDestroySampler(vk_device, global_samplers[i], nullptr);
	}

    resource_heap.Shutdown();
    sampler_heap.Shutdown();
}
