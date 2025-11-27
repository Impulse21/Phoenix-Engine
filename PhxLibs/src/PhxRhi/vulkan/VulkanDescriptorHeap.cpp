#include "PhxRhi/PhxRhi_pch.h"
#include "VulkanDescriptorHeap.h"

#include "VulkanBackend.h"
#include "VulkanGpuAllocator.h"


using namespace phx;
using namespace phx::rhi;


void phx::rhi::vulkan::DescriptorHeap::Initialize(phx::rhi::VulkanBackend* vulkan_backend, HeapType heap_type, uint32_t max_slots)
{
	m_vulkan_backend = vulkan_backend;

    VkPhysicalDeviceDescriptorBufferPropertiesEXT buffer_props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
    };

    VkPhysicalDeviceProperties2 props2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &buffer_props
    };

    vkGetPhysicalDeviceProperties2(m_vulkan_backend->vk_chosen_physical_device, &props2);

    size_t raw_size = 0;
    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (heap_type == HeapType::Resource)
    {
        raw_size = std::max(raw_size, buffer_props.sampledImageDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.storageImageDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.uniformBufferDescriptorSize);
        raw_size = std::max(raw_size, buffer_props.storageBufferDescriptorSize);

        usage_flags |= VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    }
    else // Sampler
    {
        raw_size = buffer_props.samplerDescriptorSize;
        usage_flags |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
    }

    m_descriptor_stride = AlignUp(raw_size, buffer_props.descriptorBufferOffsetAlignment);
    VkDeviceSize buffer_size = m_descriptor_stride * max_slots;


    // =====================================================================================
    // STEP 2: Configure Buffer (VMA)
    // =====================================================================================
    VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size = buffer_size;
    buffer_info.usage = usage_flags;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info = {};
    // "Auto" lets VMA decide the best heap (System RAM vs VRAM BAR)
    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    // CRITICAL FLAGS:
    // 1. HOST_ACCESS_SEQUENTIAL_WRITE: Ensures we can write to it from CPU (Host Visible).
    // 2. MAPPED: Tells VMA to map it immediately and keep it mapped (saves a vkMapMemory call).
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // Output struct to get the pointer
    VmaAllocationInfo result_info;

    // Create Buffer, Allocate Memory, Bind, and Map in one go
    vulkan_check(vmaCreateBuffer(
        m_backend->GetVmaAllocator(),
        &buffer_info,
        &alloc_info,
        &m_buffer,
        &m_allocation, // Store this member!
        &result_info
    ));

    // Get the mapped pointer directly from the result
    m_mapped_data = (char*)result_info.pMappedData;


    // =====================================================================================
    // STEP 3: Get Device Address (Still required manually)
    // =====================================================================================
    VkBufferDeviceAddressInfo address_info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    address_info.buffer = m_buffer;
    m_buffer_address = vkGetBufferDeviceAddress(m_backend->GetDevice(), &address_info);


    // =====================================================================================
    // STEP 4: Init Allocator
    // =====================================================================================
    m_slot_allocator.Initialize(max_slots);
}

void phx::rhi::vulkan::DescriptorHeap::Shutdown()
{
    if (m_vk_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vulkan_allocator->vma_allocator, m_vk_buffer, m_vma_allocation);
        m_vk_buffer = VK_NULL_HANDLE;
        m_vma_allocation = VK_NULL_HANDLE;
    }
}

rhi::DescriptorIndex phx::rhi::vulkan::DescriptorHeap::Allocate(const VkDescriptorGetInfoEXT& descriptor_info)
{
	return rhi::DescriptorIndex();
}

void phx::rhi::vulkan::DescriptorHeap::Free(uint32_t index)
{
}

const VkDescriptorSetLayout& phx::rhi::vulkan::DescriptorHeap::GetDescriptorSetLayout()
{
	// TODO: insert return statement here
}
