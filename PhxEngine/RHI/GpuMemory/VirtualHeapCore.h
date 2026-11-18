#pragma once

#include <PhxEngine/Core/PhxDefines.h>

#include <vk_mem_alloc.h>

namespace phx::rhi
{
    struct VirtualHeapAllocation
    {
        u64                  offset = 0;
        VmaVirtualAllocation handle = nullptr;

        [[nodiscard]] bool IsValid() const { return handle != nullptr; }
    };

    // General-purpose (free-list) allocation over a range of offsets
    // [0, size), backed by a VMA virtual block.
    //
    // Pure CPU bookkeeping: knows nothing about the memory the offsets point
    // into and needs no VkDevice. Concrete allocators pair it with a backing
    // (GpuCpuRange, TextureHeap) and decide *when* to free -- GPU-facing ones
    // defer it until the GPU is done. Not thread-safe.
    class VirtualHeapCore
    {
    public:
        PHX_NO_COPY(VirtualHeapCore);

        VirtualHeapCore() = default;
        ~VirtualHeapCore() { Shutdown(); }

        // Moving transfers ownership of the block; the source is left empty
        // so only one object ever destroys it.
        VirtualHeapCore(VirtualHeapCore&& other) noexcept;
        VirtualHeapCore& operator=(VirtualHeapCore&& other) noexcept;

    public:
        bool Initialize(u64 size) noexcept;

        // Releases every allocation and destroys the block. Idempotent.
        void Shutdown() noexcept;

        // Returns an invalid allocation if it doesn't fit.
        [[nodiscard]] VirtualHeapAllocation Alloc(u64 size, u64 alignment) noexcept;

        // No-op for a null handle or after Shutdown() (the block already
        // released everything).
        void Free(VmaVirtualAllocation handle) noexcept;

        // Releases every allocation, keeping the block.
        void Clear() noexcept;

        [[nodiscard]] bool IsInitialized() const noexcept { return m_block != nullptr; }
        [[nodiscard]] u64  Size()          const noexcept { return m_size; }

    private:
        VmaVirtualBlock m_block = nullptr;
        u64             m_size  = 0;
    };
}
