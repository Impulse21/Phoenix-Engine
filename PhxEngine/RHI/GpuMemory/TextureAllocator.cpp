#include "TextureAllocator.h"

#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>

using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr Log::Channel k_log = { "TextureAllocator" };
}

void phx::rhi::TextureAllocator::Initialize(TextureHeap heap) noexcept
{
    m_num_allocations = 0;
    m_storage = heap;

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

void phx::rhi::TextureAllocator::Shutdown() noexcept
{
    vmaDestroyVirtualBlock(m_virtual_block);
    m_storage = {};
}

PlacedTexture phx::rhi::TextureAllocator::Alloc(const TextureDescriptor& desc) noexcept
{
    rhi::SizeAlign texture_size_align = rhi::GetTextureSizeAlign(desc);

    VmaVirtualAllocationCreateInfo alloc_ci = {
        .size = static_cast<VkDeviceSize>(texture_size_align.size),
        .alignment = texture_size_align.align
    };

    VmaVirtualAllocation vma_alloc;
    VkDeviceSize offset;

    VkResult result = vmaVirtualAllocate(m_virtual_block, &alloc_ci, &vma_alloc, &offset);
    if (result != VK_SUCCESS)
    {
        PHX_LOG_ERROR(k_log, "Failed to allocate {0} bytes on heap", texture_size_align.size);
        return {};
    }

    TextureHandle handle = rhi::CreateTexture(desc, m_storage, offset);

    PHX_ASSERT(handle.IsValid());
    m_num_allocations++;

    return {
        .handle = handle,
        .vma_alloc = vma_alloc,
    };
}

void phx::rhi::TextureAllocator::Free(PlacedTexture& allocation) noexcept
{
    if (!allocation.handle.IsValid())
        return;

    rhi::DestroyTexture(allocation.handle);
    vmaVirtualFree(m_virtual_block, allocation.vma_alloc);

    allocation = {};
    m_num_allocations--;
}
