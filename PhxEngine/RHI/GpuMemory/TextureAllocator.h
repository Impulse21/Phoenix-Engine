#pragma once

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/VirtualHeapCore.h>

namespace phx::rhi
{
    struct PlacedTexture
    {
        TextureHandle handle;
        VmaVirtualAllocation vma_alloc = nullptr;
    };

    // Places textures into a TextureHeap (VirtualHeapCore + TextureHeap).
    //
    // Free() defers releasing the heap range until the GPU is done with the
    // frame, so a new texture can never alias memory an in-flight frame still
    // reads. The deferred callback captures `this`: the allocator is
    // non-movable and must outlive rhi::Shutdown()'s deferred-queue flush.
    // Main-thread only (deferred callbacks run on the main thread).
    class TextureAllocator
    {
    public:
        PHX_NO_COPY_NO_MOVE(TextureAllocator);
        TextureAllocator() = default;
        ~TextureAllocator() { Shutdown(); }

    public:
        void Initialize(TextureHeap heap) noexcept;

        // Releases every placement at once. Deferred frees still in flight
        // become no-ops.
        void Shutdown() noexcept;

        [[nodiscard]] PlacedTexture Alloc(const TextureDescriptor& desc) noexcept;
        void Free(PlacedTexture& allocation) noexcept;

    private:
        VirtualHeapCore m_core;
        TextureHeap     m_storage;
        u32             m_num_allocations = 0;
        u32             m_pending_frees   = 0;
    };
}
