#include "RHIVulkan.h"

#include <PhxEngine/Core/Log.h>

#include <bit>

using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::vulkan;

namespace
{
    constexpr VkMemoryPropertyFlags k_forbidden_memory_properties =
        VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT |
        VK_MEMORY_PROPERTY_PROTECTED_BIT |
        VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD |
        VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD;

    constexpr VkMemoryPropertyFlags k_cpu_visible_memory_properties =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    constexpr VkBufferUsageFlags k_always_on_usage =
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
        VkDeviceMemory  vk_memory    = VK_NULL_HANDLE;
        void*           mapped_ptr   = nullptr;
        VkDeviceAddress base_address = 0;
    };

    bool IsUsableMemoryType(const VkPhysicalDeviceMemoryProperties& properties, u32 index)
    {
        const VkMemoryType& type = properties.memoryTypes[index];
        if ((type.propertyFlags & k_forbidden_memory_properties) != 0)
            return false;

        return (properties.memoryHeaps[type.heapIndex].flags & VK_MEMORY_HEAP_TILE_MEMORY_BIT_QCOM) == 0;
    }

    bool FindMemoryType(u32   type_filter,
        VkMemoryPropertyFlags required_memory_flags,
        VkMemoryPropertyFlags preferred_memory_flags,
        VkMemoryPropertyFlags avoided_memory_flags,
        VkDeviceSize          min_heap_size,
        u32&                  output)
    {
        bool has_best        = false;
        bool best_is_avoided = false;

        u32          best           = 0;
        u32          best_score     = 0;
        VkDeviceSize best_heap_size = 0;

        const VkPhysicalDeviceMemoryProperties& vk_device_memory_properties =
            g_context.vk_physical_device_mem_properties;
        for (u32 i = 0; i < vk_device_memory_properties.memoryTypeCount; ++i)
        {
            if ((type_filter & (1u << i)) == 0)
                continue;

            const VkMemoryPropertyFlags flags = vk_device_memory_properties.memoryTypes[i].propertyFlags;
            if ((flags & required_memory_flags) != required_memory_flags)
                continue;

            if (!IsUsableMemoryType(vk_device_memory_properties, i))
                continue;

            const auto          heap_index = vk_device_memory_properties.memoryTypes[i].heapIndex;
            const VkMemoryHeap& heap       = vk_device_memory_properties.memoryHeaps[heap_index];
            if (heap.size < min_heap_size)
                continue;

            const bool is_avoided = (flags & avoided_memory_flags) != 0;
            const u32  score      = static_cast<u32>(std::popcount(flags & preferred_memory_flags));

            if (!has_best || (best_is_avoided && !is_avoided) ||
                (best_is_avoided == is_avoided &&
                    (score > best_score || (score == best_score && heap.size > best_heap_size))))
            {
                best            = i;
                has_best        = true;
                best_is_avoided = is_avoided;
                best_score      = score;
                best_heap_size  = heap.size;
            }
        }

        if (!has_best)
            return false;

        output = best;
        return true;
    }

    BackingBuffer CreateBackingBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage_flags,
        VkMemoryPropertyFlags required_memory_flags,
        VkMemoryPropertyFlags preferred_memory_flags,
        VkMemoryPropertyFlags avoided_memory_flags = 0) noexcept
    {
        BackingBuffer backing_buf;

        VkDevice vk_device = g_context.vk_device;

        VkBufferCreateInfo buffer_info = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = size,
            .usage       = usage_flags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        // -- Create Buffer ---
        vulkan_check(
            vkCreateBuffer(vk_device, &buffer_info, nullptr, &backing_buf.vk_buffer));

        // -- Buffer doesn't have any memory yet,so we must allocate the memory ---
        VkMemoryRequirements backing_mem_req;
        vkGetBufferMemoryRequirements(vk_device, backing_buf.vk_buffer, &backing_mem_req);

        /*
            https://vulkan-tutorial.com/Vertex_buffers/Vertex_buffer_creation
            The VkMemoryRequirements struct has three fields:
                size:       The size of the required amount of memory in bytes, may differ from bufferInfo.size.
                alignment:  The offset in bytes where the buffer begins in the allocated region of memory, depends 
                            on bufferInfo.usage and bufferInfo.flags.
                memoryTypeBits: Bit field of the memory types that are suitable for the buffer.

        */
        u32        memory_type     = 0;
        const bool has_memory_type = FindMemoryType(backing_mem_req.memoryTypeBits,
            required_memory_flags,
            preferred_memory_flags,
            avoided_memory_flags,
            backing_mem_req.size,
            memory_type);

        if (!has_memory_type)
        {
            PHX_LOG_ERROR(Log::Channels::RHI, "Failed to get backing device memory. Aborting");
            std::abort();
        }
        
        // Required for Device Address extension
        const VkMemoryAllocateFlagsInfo flags_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
            .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        };

        const VkMemoryAllocateInfo allocate_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &flags_info,
            .allocationSize = backing_mem_req.size,
            .memoryTypeIndex = memory_type,
        };

        vulkan_check(
            vkAllocateMemory(vk_device, &allocate_info, nullptr, &backing_buf.vk_memory)
        );
        
        vulkan_check(
            vkBindBufferMemory(vk_device, backing_buf.vk_buffer, backing_buf.vk_memory, 0)
        );

        if ((required_memory_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            vulkan_check(
                vkMapMemory(vk_device, backing_buf.vk_memory, 0, VK_WHOLE_SIZE, 0, &backing_buf.mapped_ptr)
            );
        }


        const VkBufferDeviceAddressInfo address_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = backing_buf.vk_buffer,
        };

        backing_buf.base_address = vkGetBufferDeviceAddress(vk_device, &address_info);

        return backing_buf;
    }

    constexpr VmaAllocationCreateFlags kMappedHostVisibleFlags[3] = {
        0,                                                                                    // DeviceLocal
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT, // Upload
        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,           // ReadBack
    };
}

// -- New 

[[nodiscard]] phx::rhi::GpuHeap phx::rhi::AllocateGpuHeap(u64 byte_count, phx::rhi::GpuMemoryType memory_type) noexcept
{
    // TODO: Handle Texture and Sampler heaps
    VkMemoryPropertyFlags required = 0;
    VkMemoryPropertyFlags preferred = 0;
    VkMemoryPropertyFlags avoided = 0;

    switch (memory_type)
    {
    case GpuMemoryType::CpuVisible:
        required = k_cpu_visible_memory_properties;
        break;
    case GpuMemoryType::GpuOnly:
        required = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        avoided = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        break;
    case GpuMemoryType::ReadBack:
        required = k_cpu_visible_memory_properties;
        preferred = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        break;
    default:
        assert(false && "create_gpu_heap received an invalid memory type");
        return {};
    }

    BackingBuffer backing_buffer = CreateBackingBuffer(byte_count, k_always_on_usage, required, preferred, avoided);
    BackingBuffer* internal_state = new BackingBuffer(backing_buffer);

    // TODO: Add GPU Memory Tracker to detect failed releases
    return GpuHeap {
        .range = {
            .cpu    = static_cast<byte*>(internal_state->mapped_ptr),
            .gpu    = reinterpret_cast<byte*>(static_cast<uptr>(internal_state->base_address)),
            .size   = byte_count
        },
        .internal_state = internal_state
    };
}

void phx::rhi::DestroyGpuHeap(const phx::rhi::GpuHeap& heap) noexcept
{
    if (heap.internal_state == nullptr)
        return;
    {
        VkDevice vk_device = g_context.vk_device;
        BackingBuffer* backing_buffer = static_cast<BackingBuffer*>(heap.internal_state);
        if (backing_buffer->mapped_ptr)
        {
            vkUnmapMemory(vk_device, backing_buffer->vk_memory);
            backing_buffer->mapped_ptr = nullptr;
        }
            

        vkDestroyBuffer(vk_device, backing_buffer->vk_buffer, nullptr);
        vkFreeMemory(vk_device, backing_buffer->vk_memory, nullptr);

        delete backing_buffer;
    }
}

// -- Persistent allocation (GpuMalloc arenas) ---------------------------------
// TOOD: Obsolete code path.
void phx::rhi::vulkan::InitializeGpuMemory(const rhi::InitParam& params)
{
    // TODO: REMOVE
    PHX_ASSERT(false);
#if false
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
    #endif
}

// TODO: Remove - Obsolate code path ---
void phx::rhi::vulkan::ShutdownGpuMemory()
{
    GpuTempRing& ring = g_context.gpu_temp_ring;
    if (ring.vk_buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(g_context.vma_allocator, ring.vk_buffer, ring.allocation);

        // Field-by-field, not `ring = {}` -- slot_offset is an array of
        // std::atomic, which isn't copy-assignable.
        ring.vk_buffer    = VK_NULL_HANDLE;
        ring.allocation   = VK_NULL_HANDLE;
        ring.mapped_ptr   = nullptr;
        ring.base_address = 0;
        ring.slot_size    = 0;
        for (auto& offset : ring.slot_offset)
            offset.store(0, std::memory_order_relaxed);
        ring.alignment    = 0;
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
