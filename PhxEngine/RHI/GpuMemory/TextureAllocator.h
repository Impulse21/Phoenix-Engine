#pragma once

#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/RHI/RHITypes.h>

#include <vk_mem_alloc.h>

namespace phx::rhi
{
    struct PlacedTexture
    {
        TextureHandle handle;
        VmaVirtualAllocation vma_alloc;
    };

    class TextureAllocator
    {
    public:
        static constexpr u64 k_alignment = 16;

    public:
        PHX_MOVE_ONLY(TextureAllocator);
        TextureAllocator() = default;
        ~TextureAllocator() { Shutdown(); }

    public:
        void Initialize(TextureHeap heap, u32 max_textures) noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] PlacedTexture Alloc(const TextureDescriptor& desc) noexcept;
        void Free(PlacedTexture& allocation) noexcept;

    private:
        VmaVirtualBlock m_virtual_block;
        TextureHeap m_storage;
        u32 m_max_allocations;
        u32 m_num_allocations;
    };
}