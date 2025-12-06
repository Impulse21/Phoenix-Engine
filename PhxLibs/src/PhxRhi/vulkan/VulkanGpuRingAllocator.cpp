#include "PhxRhi/PhxRhi_pch.h"

#include "VulkanGpuRingAllocator.h"

#include "VulkanInternal.h"

#include <PhxRhi/PhxRhi.h>

using namespace phx::rhi::vulkan;

void GpuRingAllocator::Initialize(VkDevice vk_device, VmaAllocator vma_allocator, uint32_t buffer_size, uint32_t block_size)
{
    // Buffer size must be a power of 2 for easy mask generation.
    if ((buffer_size & (buffer_size - 1)) != 0)
    {
        PHX_ASSERT(false, "Buffer Size must be a power of 2");
        throw std::runtime_error("Buffer Size must be a power of 2");
    }

    // Block size must also be power of 2 to ensure no partial wrap-around at the end of the physical buffer.
    if ((block_size & (block_size - 1)) != 0)
    {
		PHX_ASSERT(false, "Block Size must be a power of 2");
        throw std::runtime_error("Block Size must be a power of 2");
    }

    m_block_size = block_size;
    m_buffer_mask = buffer_size - 1;

    VkBufferCreateInfo buffer_info = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo alloc_ci = { 
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };

    m_vma_allocator = vma_allocator;

    VmaAllocationInfo alloc_info;
    VkResult result = vmaCreateBuffer(m_vma_allocator, &buffer_info, &alloc_ci, &m_buffer, &m_allocation, &alloc_info);

    if (result != VK_SUCCESS)
    {
		PHX_RHI_WARN("ReBAR GpuRingAllocator allocation failed, falling back to Host Visible memory.");
        alloc_ci.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        result = vmaCreateBuffer(m_vma_allocator, &buffer_info, &alloc_ci, &m_buffer, &m_allocation, &alloc_info);

        if (result != VK_SUCCESS)
        {
            PHX_RHI_ERROR("Failed to allocate GpuRingAllocator buffer!");
            throw std::runtime_error("Failed to allocate GpuRingAllocator buffer!");
        }
    }

    m_mapped_data = alloc_info.pMappedData;

    VkBufferDeviceAddressInfo buffer_device_address_info = { 
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = m_buffer,
    };
    m_device_address = vkGetBufferDeviceAddress(vk_device, &buffer_device_address_info);
}

void GpuRingAllocator::Shutdown()
{
    if (m_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(m_vma_allocator, m_buffer, m_allocation);

        m_buffer = VK_NULL_HANDLE;
        m_allocation = VK_NULL_HANDLE;
        m_mapped_data = nullptr;
        m_device_address = 0;
    }
    m_in_use_regions.clear();
    m_head = 0;
    m_tail = 0;
    m_frame_start_tail = 0;

}

GpuRingAllocator::GpuMemoryBlock GpuRingAllocator::GetNextMemoryBlock()
{
    if ((m_tail - m_head) + m_block_size > GetBufferSize())
    {
        PHX_ASSERT(false, "GpuRingAllocator Overflow! Increase Buffer Size.");
        return {};
    }

    const uint64_t offset = m_tail & m_buffer_mask;
    m_tail += m_block_size;

    return GpuMemoryBlock{
        .device_address = m_device_address + offset,
        .mapped_data = static_cast<uint8_t*>(m_mapped_data) + offset,
    };
}

void phx::rhi::vulkan::GpuRingAllocator::BeginFrame(uint64_t completed_fence_value)
{
    std::scoped_lock lock(m_mutex);

    while (!m_in_use_regions.empty())
    {
        UsedRegion& region = m_in_use_regions.front();
        if (completed_fence_value >= region.fence_value)
        {
            m_head += region.used_size;
            m_in_use_regions.pop_front();
        }
        else
        {
            break;
        }
    }
}

void GpuRingAllocator::EndFrame(uint64_t frame_completion_value)
{
    if (m_tail == m_frame_start_tail) 
        return;

    // Since m_tail is monotonic, this is always simple subtraction.
    // The "wrap" logic happens virtually via the mask during access.
    uint64_t used_size = m_tail - m_frame_start_tail;

    m_in_use_regions.push_back({ used_size, frame_completion_value });
    m_frame_start_tail = m_tail;
}
