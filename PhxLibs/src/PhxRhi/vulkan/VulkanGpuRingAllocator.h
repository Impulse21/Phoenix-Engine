#pragma once


#include <vk_mem_alloc.h>
#include <volk.h>
#include <deque>
#include <mutex>
#include <vector>

#include <PhxRhi/PhxRhi_Types.h>

namespace phx::rhi::vulkan
{
    class GpuRingAllocator
    {
    public:
        struct GpuMemoryBlock
        {
            VkDeviceAddress device_address = 0;
            uint8_t* mapped_data = nullptr;

            bool IsValid() const { return mapped_data != nullptr; }
        };

        void Initialize(VkDevice vk_device, VmaAllocator vma_allocator, uint32_t buffer_size, uint32_t block_size);
        void Shutdown();

        void BeginFrame(uint64_t completed_fence_value);
        void EndFrame(uint64_t frame_completion_value);

        [[nodiscard]] GpuMemoryBlock GetNextMemoryBlock();

        uint32_t GetBufferSize() const { return (m_buffer_mask + 1); }
        uint32_t GetBlockSize() const { return m_block_size; }

    private:
        struct UsedRegion
        {
            uint64_t     used_size = 0;
			uint64_t     fence_value = 0;   
        };

        std::mutex m_mutex;

        VmaAllocator m_vma_allocator = VK_NULL_HANDLE;

        uint32_t m_block_size = 0;
        uint64_t m_buffer_mask = 0;

        uint64_t m_head = 0;
        uint64_t m_tail = 0;
        uint64_t m_frame_start_tail = 0;

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;

        void* m_mapped_data = nullptr;
        VkDeviceAddress m_device_address = 0;

        std::deque<UsedRegion>     m_in_use_regions;
    };


    struct TempAllocation
    {
        size_t          byte_offset;
        uint8_t*        mapped_data;
        VkDeviceAddress device_address;
    };

    // A linear allocator that sub-allocates from blocks provided by GpuRingAllocator.
    struct GpuLinearAllocator
    {
        [[nodiscard]] TempAllocation Allocate(GpuRingAllocator& main_allocator, uint32_t byte_size, uint32_t alignment)
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

            return {
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

        GpuRingAllocator::GpuMemoryBlock m_current_block;
        uint32_t m_byte_offset = 0;
    };
}