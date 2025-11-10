#include "PhxRhi/PhxRhi_pch.h"
#include "VkGpuTempMemory.h"

#include "VkRhi_Internal.h"

#include <PhxRhi/PhxRhi.h>
using namespace phx::rhi::vk;

void GpuTempMemoryAllocator::Initialize(uint32_t buffer_size, uint32_t block_size)
{
    // Buffer size must be a power of 2 for easy mask generation.
    if ((buffer_size & (buffer_size - 1)) != 0)
    {
        throw std::runtime_error("Buffer Size must be a power of 2");
    }

    m_block_size = block_size;
    m_buffer_mask = buffer_size - 1;

    // Create the GPU buffer
    m_buffer = rhi::CreateBuffer({
            .Size = buffer_size,
            .Usage = rhi::Usage::Dynamic,
            .BindingFlags = rhi::BindingFlags::ShaderResource,
            .MiscFlags = rhi::ResourceMiscFlags::BufferRaw,
            .InitialState = rhi::ResourceStates::CopySource,
        });

    rhi::Buffer_VK* internal = rhi::VkContext::buffer_pool.GetHot(m_buffer);
    m_mapped_data = internal->mapped_data;
    m_device_address = internal->gpu_address;
}

void GpuTempMemoryAllocator::Shutdown()
{
    if (m_buffer.IsValid())
        return;

    std::scoped_lock lock(m_mutex);

    VkDevice logical_device = rhi::VkContext::vk_device;

    // Wait for any remaining in-flight fences to complete.
    while (!m_in_use_regions.empty())
    {
        UsedRegion& region = m_in_use_regions.front();
        vkWaitForFences(logical_device, 1, &region.fence, VK_TRUE, UINT64_MAX);
        m_in_use_regions.pop_front();
    }

    // Manually destroy all fences in the pool.
    for (VkFence fence : m_fence_pool)
    {
        vkDestroyFence(logical_device, fence, nullptr);
    }
    m_fence_pool.clear();

    rhi::DeleteBuffer(m_buffer);
    m_buffer = {};
}

GpuTempMemoryAllocator::GpuMemoryBlock GpuTempMemoryAllocator::GetNextMemoryBlock()
{
    std::scoped_lock lock(m_mutex);
    if ((m_tail - m_head) + m_block_size > GetBufferSize())
    {
        WaitForFreeRegions();
    }

    const uint64_t offset = m_tail & m_buffer_mask;
    m_tail += m_block_size;

    return GpuMemoryBlock{
        .device_address = m_device_address + offset,
        .mapped_data = static_cast<uint8_t*>(m_mapped_data) + offset,
    };
}

void GpuTempMemoryAllocator::EndFrame(VkQueue queue)
{
    std::scoped_lock lock(m_mutex);

    const uint64_t used_size = m_tail - m_frame_start_tail;
    if (used_size == 0)
    {
        m_frame_start_tail = m_tail;
        return;
    }

    VkDevice logical_device = rhi::VkContext::vk_device;
    while (!m_in_use_regions.empty())
    {
        UsedRegion& region = m_in_use_regions.front();

        if (vkGetFenceStatus(logical_device, region.fence) != VK_SUCCESS)
        {
            break;
        }

        vkResetFences(logical_device, 1, &region.fence);
        m_available_fences.push_back(region.fence);

        // Advance the head pointer to free up space.
        m_head += region.used_size;
        m_in_use_regions.pop_front();
    }

    VkFence fence_to_signal = VK_NULL_HANDLE;
    if (!m_available_fences.empty())
    {
        fence_to_signal = m_available_fences.front();
        m_available_fences.pop_front();
    }
    else
    {
        VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkFence new_fence = VK_NULL_HANDLE;
        if (vkCreateFence(logical_device, &fence_create_info, nullptr, &new_fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create fence!");
        }
        m_fence_pool.push_back(new_fence);
        fence_to_signal = new_fence;
    }

    vkQueueSubmit(queue, 0, nullptr, fence_to_signal);

    m_in_use_regions.push_back({ .used_size = used_size, .fence = fence_to_signal });
    m_frame_start_tail = m_tail;
}

void GpuTempMemoryAllocator::WaitForFreeRegions()
{
    if (m_in_use_regions.empty())
    {
        // This should not be possible if the space check is correct.
        throw std::runtime_error("No regions to free, but buffer is full!");
    }

    VkDevice logical_device = rhi::VkContext::vk_device;
    UsedRegion& oldest_region = m_in_use_regions.front();

    vkWaitForFences(logical_device, 1, &oldest_region.fence, VK_TRUE, UINT64_MAX);

    vkResetFences(logical_device, 1, &oldest_region.fence);
    m_available_fences.push_back(oldest_region.fence);

    m_head += oldest_region.used_size;
    m_in_use_regions.pop_front();
}