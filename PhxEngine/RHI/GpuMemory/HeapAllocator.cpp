#include "HeapAllocator.h"

#include <PhxEngine/Core/Log.h>

using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr Log::Channel k_log = { "Gpu Heap Allocator" };
}

void phx::rhi::GpuHeapAllocator::Initialize(GpuCpuRange<byte> block) noexcept
{
    m_storage = block;

    VmaVirtualBlockCreateInfo block_create_info = {
        .size = m_storage.size
    };

    VkResult result = vmaCreateVirtualBlock(&block_create_info, &m_virtual_block);
    if (result != VK_SUCCESS) 
    {
        PHX_LOG_ERROR(k_log, "Failed to create virtual block");
        return;
    }
}

void phx::rhi::GpuHeapAllocator::Shutdown() noexcept
{
}

HeapAllocation<byte> phx::rhi::GpuHeapAllocator::Alloc(u64 size) noexcept
{
    VmaVirtualAllocationCreateInfo alloc_ci = {
        .size = static_cast<VkDeviceSize>(size),
        .alignment = k_alignment
    };

    VmaVirtualAllocation vma_alloc;
    VkDeviceSize offset;

    VkResult result = vmaVirtualAllocate(m_virtual_block, &alloc_ci, &vma_alloc, &offset);
    if (result != VK_SUCCESS) 
    {
        PHX_LOG_ERROR(k_log, "Failed to allocate {0} bytes on heap", size);
        return {};
    }

    return {
        .range = {
            .cpu = OffsetPointer(m_storage.cpu, offset),
            .gpu = OffsetPointer(m_storage.gpu, offset),
            .size = size,
        },
        .vma_alloc = vma_alloc,
    };
}

void phx::rhi::GpuHeapAllocator::Reset() noexcept
{
    vmaClearVirtualBlock(m_virtual_block);
}
