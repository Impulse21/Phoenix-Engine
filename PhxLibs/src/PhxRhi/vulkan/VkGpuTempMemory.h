#pragma once

#include <volk.h>
#include <deque>
#include <mutex>
#include <vector>

#include <PhxRhi/RHICommon.h>

namespace phx::rhi::vk
{
    class GpuTempMemoryAllocator
    {
    public:
        struct GpuMemoryBlock
        {
            VkDeviceAddress device_address = 0;
            uint8_t* mapped_data = nullptr;

            bool IsValid() const { return mapped_data != nullptr; }
        };

        // Represents a sub-allocation within a GpuMemoryBlock.
        struct TempBuffer
        {
            size_t          byte_offset;
            uint8_t* mapped_data;
            VkDeviceAddress device_address;
        };

        void Initialize(uint32_t buffer_size, uint32_t block_size);
        void Shutdown();

        void EndFrame(VkQueue queue);

        [[nodiscard]] GpuMemoryBlock GetNextMemoryBlock();

        uint32_t GetBufferSize() const { return (m_buffer_mask + 1); }
        uint32_t GetBlockSize() const { return m_block_size; }

    private:
        void WaitForFreeRegions();

    private:
        // A region of the buffer used in a single frame, tracked by a fence.
        struct UsedRegion
        {
            uint64_t     used_size = 0;
            VkFence      fence = VK_NULL_HANDLE;
        };

        std::mutex m_mutex;

        // Allocator configuration
        uint32_t m_block_size = 0;
        uint64_t m_buffer_mask = 0;

        // Ring buffer state using monotonically increasing head/tail
        uint64_t m_head = 0;
        uint64_t m_tail = 0;
        uint64_t m_frame_start_tail = 0;

        // Vulkan objects
        rhi::GpuBufferHandle m_buffer;
        void* m_mapped_data = nullptr;
        VkDeviceAddress m_device_address = 0;

        // Fence management
        std::vector<VkFence>       m_fence_pool;
        std::deque<VkFence>        m_available_fences;
        std::deque<UsedRegion>     m_in_use_regions;
    };


    // A linear allocator that sub-allocates from blocks provided by GpuTempMemoryAllocator.
    struct TempSubAllocator
    {
        // Allocates a small piece of memory for a single resource.
        [[nodiscard]] GpuTempMemoryAllocator::TempBuffer Allocate(GpuTempMemoryAllocator& main_allocator, uint32_t byte_size, uint32_t alignment)
        {
            // Align the current offset up to the required alignment.
            uint32_t aligned_offset = (m_byte_offset + alignment - 1) & ~(alignment - 1);

            // If the current block is invalid or doesn't have enough space, get a new one.
            if (!m_current_block.IsValid() || (aligned_offset + byte_size) > main_allocator.GetBlockSize())
            {
                m_current_block = main_allocator.GetNextMemoryBlock();
                aligned_offset = 0;
            }

            m_byte_offset = aligned_offset + byte_size;

            return GpuTempMemoryAllocator::TempBuffer{
                .byte_offset = aligned_offset,
                .mapped_data = m_current_block.mapped_data + aligned_offset,
                .device_address = m_current_block.device_address + aligned_offset,
            };
        }

        void Reset()
        {
            m_byte_offset = 0;
            m_current_block = {};
        }

        GpuTempMemoryAllocator::GpuMemoryBlock m_current_block;
        uint32_t m_byte_offset = 0;
    };
}