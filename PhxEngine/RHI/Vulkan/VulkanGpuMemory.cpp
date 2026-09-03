#include "RHIVulkan.h"

#include <PhxEngine/Core/Log.h>

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

namespace
{
    // A GpuMalloc'd buffer carries no descriptor/binding intent up front —
    // the caller decides what it means once it has the pointer — so every
    // allocation supports every use.
    constexpr VkBufferUsageFlags kAlwaysOnUsage =
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // Shared by the GpuTempMalloc ring and every GpuMalloc arena — creates
    // the backing VkBuffer/VmaAllocation and, for host-visible usages, a
    // persistent mapping, then resolves its BDA base address.
    struct BackingBuffer
    {
        VkBuffer        vk_buffer    = VK_NULL_HANDLE;
        VmaAllocation   allocation   = VK_NULL_HANDLE;
        char*           mapped_ptr   = nullptr;
        VkDeviceAddress base_address = 0;
    };

    BackingBuffer CreateBackingBuffer(VkDeviceSize size, VmaAllocationCreateFlags vma_flags)
    {
        BackingBuffer buf;

        VkBufferCreateInfo buffer_info = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = size,
            .usage       = kAlwaysOnUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VmaAllocationCreateInfo alloc_info = {
            .flags = vma_flags,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        VmaAllocationInfo vma_alloc_info;
        vulkan_check(
            vmaCreateBuffer(g_context.vma_allocator, &buffer_info, &alloc_info,
                &buf.vk_buffer, &buf.allocation, &vma_alloc_info));

        buf.mapped_ptr = static_cast<char*>(vma_alloc_info.pMappedData);

        VkBufferDeviceAddressInfo address_info = {
            .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buf.vk_buffer,
        };
        buf.base_address = vkGetBufferDeviceAddress(g_context.vk_device, &address_info);

        return buf;
    }

    constexpr VmaAllocationCreateFlags kMappedHostVisibleFlags[3] = {
        0,                                                                                    // DeviceLocal
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, // Upload
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,           // ReadBack
    };
}

// -- Persistent allocation (GpuMalloc arenas) ---------------------------------

void phx::rhi::vulkan::InitializeGpuMemory(const rhi::InitParam& params)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_context.vk_physical_device, &props);
    usize alignment = static_cast<usize>(
        props.limits.minStorageBufferOffsetAlignment > props.limits.minUniformBufferOffsetAlignment
            ? props.limits.minStorageBufferOffsetAlignment
            : props.limits.minUniformBufferOffsetAlignment);
    if (alignment == 0)
        alignment = 1;

    // -- GpuTempMalloc ring ---
    GpuTempRing& ring = g_context.gpu_temp_ring;
    ring.alignment = alignment;
    ring.slot_size = params.gpu_temp_ring_size;

    BackingBuffer ring_buf = CreateBackingBuffer(
        static_cast<VkDeviceSize>(ring.slot_size) * rhi::MaxFramesInFlight,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    ring.vk_buffer    = ring_buf.vk_buffer;
    ring.allocation   = ring_buf.allocation;
    ring.mapped_ptr   = ring_buf.mapped_ptr;
    ring.base_address = ring_buf.base_address;

    PHX_LOG_INFO(Log::Channels::RHI, "GpuTempMalloc ring — {} bytes/frame, {} bytes total",
        ring.slot_size, ring.slot_size * rhi::MaxFramesInFlight);

    // -- GpuMalloc arenas ---
    const u32 arena_sizes[3] = {
        params.gpu_arena_size_device_local,
        params.gpu_arena_size_upload,
        params.gpu_arena_size_readback,
    };

    for (u32 i = 0; i < 3; ++i)
    {
        GpuArena& arena = g_context.gpu_arenas[i];
        arena.size      = arena_sizes[i];
        arena.alignment = alignment;

        BackingBuffer buf = CreateBackingBuffer(arena.size, kMappedHostVisibleFlags[i]);
        arena.vk_buffer    = buf.vk_buffer;
        arena.allocation   = buf.allocation;
        arena.mapped_ptr   = buf.mapped_ptr;
        arena.base_address = buf.base_address;

        VmaVirtualBlockCreateInfo block_info = {
            .size = arena.size,
        };
        vulkan_check(
            vmaCreateVirtualBlock(&block_info, &arena.virtual_block));

        PHX_LOG_INFO(Log::Channels::RHI, "GpuMalloc arena[{}] — {} bytes", i, arena.size);
    }

    // -- GpuUploadMalloc ring ---
    GpuUploadRing& upload_ring = g_context.gpu_upload_ring;
    upload_ring.alignment = alignment;
    upload_ring.slot_size = params.gpu_upload_ring_slot_size;

    BackingBuffer upload_buf = CreateBackingBuffer(
        static_cast<VkDeviceSize>(upload_ring.slot_size) * GpuUploadRing::kSlotCount,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

    upload_ring.vk_buffer    = upload_buf.vk_buffer;
    upload_ring.allocation   = upload_buf.allocation;
    upload_ring.mapped_ptr   = upload_buf.mapped_ptr;
    upload_ring.base_address = upload_buf.base_address;

    PHX_LOG_INFO(Log::Channels::RHI, "GpuUploadMalloc ring — {} bytes/slot, {} slots, {} bytes total",
        upload_ring.slot_size, GpuUploadRing::kSlotCount, upload_ring.slot_size * GpuUploadRing::kSlotCount);
}

void phx::rhi::vulkan::ShutdownGpuMemory()
{
    GpuTempRing& ring = g_context.gpu_temp_ring;
    if (ring.vk_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(g_context.vma_allocator, ring.vk_buffer, ring.allocation);
        ring = {};
    }

    for (GpuArena& arena : g_context.gpu_arenas)
    {
        if (arena.virtual_block != VK_NULL_HANDLE)
            vmaDestroyVirtualBlock(arena.virtual_block);

        if (arena.vk_buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(g_context.vma_allocator, arena.vk_buffer, arena.allocation);

        arena = {};
    }

    GpuUploadRing& upload_ring = g_context.gpu_upload_ring;
    if (upload_ring.vk_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(g_context.vma_allocator, upload_ring.vk_buffer, upload_ring.allocation);
        upload_ring = {};
    }
}

rhi::GpuAllocation phx::rhi::GpuMalloc(u32 size, GpuMemoryUsage usage)
{
    GpuArena& arena = g_context.gpu_arenas[static_cast<u32>(usage)];

    VmaVirtualAllocationCreateInfo alloc_info = {
        .size      = size,
        .alignment = arena.alignment,
    };

    VmaVirtualAllocation handle;
    VkDeviceSize          offset;
    const VkResult result = vmaVirtualAllocate(arena.virtual_block, &alloc_info, &handle, &offset);
    if (result != VK_SUCCESS)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "GpuMalloc ran out of arena space (usage={})", static_cast<u32>(usage));
        PHX_ASSERT(false);
        return {};
    }

    return GpuAllocation{
        .internal_state = reinterpret_cast<void*>(handle),
        .cpu_ptr        = arena.mapped_ptr ? arena.mapped_ptr + offset : nullptr,
        .gpu_address    = arena.base_address + offset,
        .size           = size,
    };
}

void phx::rhi::GpuFree(const GpuAllocation& allocation)
{
    if (!allocation.IsValid())
        return;

    GpuArena* owning_arena = nullptr;
    for (GpuArena& arena : g_context.gpu_arenas)
    {
        if (allocation.gpu_address >= arena.base_address &&
            allocation.gpu_address < arena.base_address + arena.size)
        {
            owning_arena = &arena;
            break;
        }
    }

    PHX_ASSERT(owning_arena);
    if (!owning_arena)
        return;

    VmaVirtualAllocation handle = reinterpret_cast<VmaVirtualAllocation>(allocation.internal_state);

    g_context.deferred_callback_queue.EnqueueDelete({
        .frame = g_context.frame_number,
        .deferred_func = [owning_arena, handle]() {
            vmaVirtualFree(owning_arena->virtual_block, handle);
        }
    });
}

// -- Per-frame ring -----------------------------------------------------------

rhi::GpuAllocation phx::rhi::GpuTempMalloc(u32 size)
{
    GpuTempRing& ring = g_context.gpu_temp_ring;

    const usize aligned_size = (static_cast<usize>(size) + ring.alignment - 1) & ~(ring.alignment - 1);
    const u64   frame_slot   = g_context.GetCurrentFrame();

    if (ring.slot_offset[frame_slot] + aligned_size > ring.slot_size)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "GpuTempMalloc ran out of ring space for this frame");
        PHX_ASSERT(false);
        return {};
    }

    const usize offset_in_slot = ring.slot_offset[frame_slot];
    ring.slot_offset[frame_slot] += aligned_size;

    const usize absolute_offset = static_cast<usize>(frame_slot) * ring.slot_size + offset_in_slot;

    return GpuAllocation{
        .internal_state = nullptr, // never freed individually
        .cpu_ptr        = ring.mapped_ptr + absolute_offset,
        .gpu_address    = ring.base_address + absolute_offset,
        .size           = size,
    };
}

// -- Upload ring ----------------------------------------------------------

rhi::GpuAllocation phx::rhi::GpuUploadMalloc(u32 size)
{
    GpuUploadRing& ring = g_context.gpu_upload_ring;
    const u32 slot = ring.current_slot;

    // Lazily reclaim this slot the first time it's written into since it was
    // last closed out by SubmitUpload — blocks only if that submission's GPU
    // work hasn't finished yet.
    if (ring.slot_needs_wait[slot])
    {
        rhi::WaitForUpload(ring.slot_ticket[slot]);
        ring.slot_offset[slot] = 0;
        ring.slot_needs_wait[slot] = false;
    }

    const usize aligned_size = (static_cast<usize>(size) + ring.alignment - 1) & ~(ring.alignment - 1);

    if (ring.slot_offset[slot] + aligned_size > ring.slot_size)
    {
        PHX_LOG_ERROR(Log::Channels::RHI, "GpuUploadMalloc ran out of ring space for this slot");
        PHX_ASSERT(false);
        return {};
    }

    const usize offset_in_slot = ring.slot_offset[slot];
    ring.slot_offset[slot] += aligned_size;

    const usize absolute_offset = static_cast<usize>(slot) * ring.slot_size + offset_in_slot;

    return GpuAllocation{
        .internal_state = nullptr, // never freed individually
        .cpu_ptr        = ring.mapped_ptr + absolute_offset,
        .gpu_address    = ring.base_address + absolute_offset,
        .size           = size,
    };
}
