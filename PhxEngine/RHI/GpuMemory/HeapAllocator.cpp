#include "HeapAllocator.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/Thread.h>
#include <PhxEngine/RHI/RHI.h>

using namespace phx;
using namespace phx::rhi;

namespace
{
    constexpr Log::Channel k_log = { "Gpu Heap Allocator" };
}

void phx::rhi::GpuHeapAllocator::Initialize(GpuCpuRange<byte> block) noexcept
{
    PHX_ASSERT(m_pending_frees == 0 && "Re-initialized while frees from the previous block are still deferred");

    m_storage = block;
    m_core.Initialize(block.size);
}

void phx::rhi::GpuHeapAllocator::Shutdown() noexcept
{
    m_core.Shutdown();
    m_storage = {};
}

HeapAllocation<byte> phx::rhi::GpuHeapAllocator::Alloc(u64 size) noexcept
{
    PHX_ASSERT(Thread::IsMainThread());

    const VirtualHeapAllocation allocation = m_core.Alloc(size, k_alignment);
    if (!allocation.IsValid())
    {
        PHX_LOG_ERROR(k_log, "Failed to allocate {0} bytes on heap", size);
        return {};
    }

    return {
        .range = {
            .cpu = OffsetPointer(m_storage.cpu, allocation.offset),
            .gpu = OffsetPointer(m_storage.gpu, allocation.offset),
            .size = size,
        },
        .vma_alloc = allocation.handle,
    };
}

void phx::rhi::GpuHeapAllocator::FreeDeferred(VmaVirtualAllocation vma_alloc) noexcept
{
    PHX_ASSERT(Thread::IsMainThread());
    if (vma_alloc == nullptr)
        return;

    m_pending_frees++;
    rhi::DeferUntilGpuComplete([this, vma_alloc]
    {
        m_pending_frees--;

        // No-op after Shutdown(): the block already released everything.
        m_core.Free(vma_alloc);
    });
}

void phx::rhi::GpuHeapAllocator::Reset() noexcept
{
    PHX_ASSERT(m_pending_frees == 0 && "Reset while deferred frees are pending");
    m_core.Clear();
}
