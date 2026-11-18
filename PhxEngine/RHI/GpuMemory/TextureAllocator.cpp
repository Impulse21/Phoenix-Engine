#include "TextureAllocator.h"

#include <PhxEngine/RHI/RHI.h>

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Thread.h>

using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr Log::Channel k_log = { "TextureAllocator" };
}

void phx::rhi::TextureAllocator::Initialize(TextureHeap heap) noexcept
{
    PHX_ASSERT(m_pending_frees == 0 && "Re-initialized while frees from the previous heap are still deferred");

    m_num_allocations = 0;
    m_storage = heap;
    m_core.Initialize(heap.size);
}

void phx::rhi::TextureAllocator::Shutdown() noexcept
{
    m_core.Shutdown();
    m_storage = {};
    m_num_allocations = 0;
}

PlacedTexture phx::rhi::TextureAllocator::Alloc(const TextureDescriptor& desc) noexcept
{
    PHX_ASSERT(Thread::IsMainThread());
    const rhi::SizeAlign texture_size_align = rhi::GetTextureSizeAlign(desc);

    const VirtualHeapAllocation allocation = m_core.Alloc(texture_size_align.size, texture_size_align.align);
    if (!allocation.IsValid())
    {
        PHX_LOG_ERROR(k_log, "Failed to allocate {0} bytes on heap", texture_size_align.size);
        return {};
    }

    TextureHandle handle = rhi::CreateTexture(desc, m_storage, allocation.offset);
    if (!handle.IsValid())
    {
        // Nothing was placed, so the GPU can't be using the range: release it now.
        PHX_LOG_ERROR(k_log, "Failed to create placed texture");
        m_core.Free(allocation.handle);
        return {};
    }

    m_num_allocations++;

    return {
        .handle = handle,
        .vma_alloc = allocation.handle,
    };
}

void phx::rhi::TextureAllocator::Free(PlacedTexture& allocation) noexcept
{
    PHX_ASSERT(Thread::IsMainThread());
    if (!allocation.handle.IsValid())
        return;

    // DestroyTexture defers the VkImage itself. The heap range must be
    // deferred too, or the next Alloc could place a texture over memory the
    // GPU is still reading.
    rhi::DestroyTexture(allocation.handle);

    m_pending_frees++;
    rhi::DeferUntilGpuComplete([this, vma_alloc = allocation.vma_alloc]
    {
        m_pending_frees--;

        // No-op after Shutdown(): the block already released everything.
        m_core.Free(vma_alloc);
    });

    allocation = {};
    m_num_allocations--;
}
