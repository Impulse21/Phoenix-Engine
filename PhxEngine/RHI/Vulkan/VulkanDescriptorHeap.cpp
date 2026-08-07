
#include "VulkanDescriptorHeap.h"

#include "RHIVulkan.h"
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Memory/MemoryHelpers.h>

using namespace phx;
using namespace phx::rhi;


void phx::rhi::vulkan::DescriptorHeap::Initialize(
    VkDevice vk_device,
    VmaAllocator vma_allocator,
    VkPhysicalDevice vk_physical_device,
    HeapType heap_type,
    uint32_t max_slots)
{
    m_vk_device = vk_device;
    m_vma_allocator = vma_allocator;

    VkPhysicalDeviceDescriptorBufferPropertiesEXT buffer_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
    };

    VkPhysicalDeviceProperties2 props2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &buffer_props
    };

    vkGetPhysicalDeviceProperties2(vk_physical_device, &props2);

    size_t raw_size = 0;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (heap_type == HeapType::Resource)
    {
        PHX_LOG_INFO(Log::Channels::RHI, "Resource heap - Sampled Image descriptor Size {0}", buffer_props.sampledImageDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.sampledImageDescriptorSize);

        PHX_LOG_INFO(Log::Channels::RHI, "Resource heap - Storaged Image descriptor Size {0}", buffer_props.storageImageDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.storageImageDescriptorSize);

#if false
        PHX_LOG_INFO(Log::Channels::RHI, "Resource heap - Uniform Buffer descriptor Size {0}", buffer_props.uniformBufferDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.uniformBufferDescriptorSize);

        PHX_LOG_INFO(Log::Channels::RHI, "Resource heap - Stroage Buffer descriptor Size {0}", buffer_props.storageBufferDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.storageBufferDescriptorSize);
#else
        PHX_LOG_INFO(Log::Channels::RHI, "Resource heap - Bindless buffers are disabled.");
#endif
        usage_flags |= VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    }
    else // Sampler
    {
        PHX_LOG_INFO(Log::Channels::RHI, "Sampler Heap - Descriptor Stride {0}", buffer_props.samplerDescriptorSize);
        raw_size = buffer_props.samplerDescriptorSize;
        usage_flags |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    PHX_LOG_INFO(Log::Channels::RHI, "Descriptor Heap Storage Stride {0}", raw_size);
    m_descriptor_stride = raw_size;

    const VkDeviceSize buffer_size = m_descriptor_stride * max_slots;

    PHX_LOG_INFO(Log::Channels::RHI, "Descriptor heap size is {0} bytes - {1} KB", buffer_size, PhxToKB(buffer_size));

    // -- Create the buffer for the descriptor heap
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer_size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo alloc_info = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo result_info;
    vulkan_check(
        vmaCreateBuffer(
            m_vma_allocator,
            &buffer_info,
            &alloc_info,
            &m_vk_buffer,
            &m_vma_allocation,
            &result_info
    ));

    m_mapped_ptr = (char*)result_info.pMappedData;

    VkBufferDeviceAddressInfo address_info = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = m_vk_buffer, 
    };

    m_buffer_address = vkGetBufferDeviceAddress(vk_device, &address_info);
    m_slot_allocator.Initialize(max_slots);
}

void phx::rhi::vulkan::DescriptorHeap::Shutdown()
{
    if (m_vk_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(
            m_vma_allocator, m_vk_buffer, m_vma_allocation);
        m_vk_buffer = VK_NULL_HANDLE;
        m_vma_allocation = VK_NULL_HANDLE;
    }
}

rhi::DescriptorIndex phx::rhi::vulkan::DescriptorHeap::Allocate(const VkDescriptorGetInfoEXT& descriptor_info)
{
    rhi::DescriptorIndex index = m_slot_allocator.AllocateSlot();

    if (index == rhi::cInvalidDescriptorIndex)
    {
        PHX_RHI_CRITICAL("Out of descriptor heap memory");
        return rhi::cInvalidDescriptorIndex;
    }

    char* dest_ptr = m_mapped_ptr + (index * m_descriptor_stride);

    vkGetDescriptorEXT(
        m_vk_device,
        &descriptor_info,
        m_descriptor_stride,
        dest_ptr
    );

    return index;
}

void phx::rhi::vulkan::DescriptorHeap::Free(uint32_t index)
{
    m_slot_allocator.FreeSlot(index);
}
