#include "VirtualHeapCore.h"

#include <PhxEngine/Core/Log.h>

#include <utility>

using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr Log::Channel k_log = { "VirtualHeapCore" };
}

phx::rhi::VirtualHeapCore::VirtualHeapCore(VirtualHeapCore&& other) noexcept
    : m_block(std::exchange(other.m_block, nullptr))
    , m_size(std::exchange(other.m_size, 0))
{
}

VirtualHeapCore& phx::rhi::VirtualHeapCore::operator=(VirtualHeapCore&& other) noexcept
{
    if (this != &other)
    {
        Shutdown();
        m_block = std::exchange(other.m_block, nullptr);
        m_size  = std::exchange(other.m_size, 0);
    }
    return *this;
}

bool phx::rhi::VirtualHeapCore::Initialize(u64 size) noexcept
{
    PHX_ASSERT(m_block == nullptr && "VirtualHeapCore initialized twice");

    const VmaVirtualBlockCreateInfo block_create_info = {
        .size = static_cast<VkDeviceSize>(size),
    };

    const VkResult result = vmaCreateVirtualBlock(&block_create_info, &m_block);
    if (result != VK_SUCCESS)
    {
        PHX_LOG_ERROR(k_log, "Failed to create virtual block of {0} bytes", size);
        m_block = nullptr;
        return false;
    }

    m_size = size;
    return true;
}

void phx::rhi::VirtualHeapCore::Shutdown() noexcept
{
    if (m_block == nullptr)
        return;

    // Clear first: VMA asserts if a virtual block is destroyed with live
    // allocations.
    vmaClearVirtualBlock(m_block);
    vmaDestroyVirtualBlock(m_block);
    m_block = nullptr;
    m_size  = 0;
}

VirtualHeapAllocation phx::rhi::VirtualHeapCore::Alloc(u64 size, u64 alignment) noexcept
{
    PHX_ASSERT(m_block != nullptr);
    PHX_ASSERT(alignment != 0 && (alignment & (alignment - 1)) == 0);

    const VmaVirtualAllocationCreateInfo alloc_ci = {
        .size      = static_cast<VkDeviceSize>(size),
        .alignment = static_cast<VkDeviceSize>(alignment),
    };

    VmaVirtualAllocation handle = nullptr;
    VkDeviceSize offset = 0;
    if (vmaVirtualAllocate(m_block, &alloc_ci, &handle, &offset) != VK_SUCCESS)
        return {};

    return {
        .offset = static_cast<u64>(offset),
        .handle = handle,
    };
}

void phx::rhi::VirtualHeapCore::Free(VmaVirtualAllocation handle) noexcept
{
    if (handle == nullptr || m_block == nullptr)
        return;

    vmaVirtualFree(m_block, handle);
}

void phx::rhi::VirtualHeapCore::Clear() noexcept
{
    PHX_ASSERT(m_block != nullptr);
    vmaClearVirtualBlock(m_block);
}
