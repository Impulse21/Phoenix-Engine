#pragma once

#include <PhxEngine/Core/StaticArray.h>
#include <PhxEngine/Memory/BumpCore.h>
#include <PhxEngine/RHI/RHITypes.h>

namespace phx::rhi
{
    class DescriptorAllocator;
}

namespace phx::renderer
{

    enum class ResourceKind : u8
    {
        Texture,
        Buffer
    };

    struct TextureDesc
    {
        const char* debug_name  = "";
        u32 width               = 1;
        u32 height              = 1;
        rhi::Format format      = rhi::Format::UNKNOWN;
    };

    struct BufferDesc
    {
        usize sizeBytes;
        const char* debug_name;
    };

    struct ResourceDesc
    {
        ResourceKind kind = ResourceKind::Texture;
        union
        {
            TextureDesc texture_desc;
            BufferDesc buffer_desc;
        };

        static ResourceDesc Texture(TextureDesc t)
        {
            ResourceDesc d = {};
            d.kind = ResourceKind::Texture;
            d.texture_desc = t;
            return d;
        }
        
        static ResourceDesc Buffer(BufferDesc b)
        {
            ResourceDesc d = {};
            d.kind = ResourceKind::Buffer;
            d.buffer_desc = b;
            return d;
        }
    };

    struct TransientTexture
    {
        rhi::TextureHandle   handle;
        rhi::DescriptorIndex descriptor = rhi::kInvalidDescriptorIndex;   // Only set for ShaderResource textures.
    };

    // Owns the render graph's transient textures.
    //
    // Memory: textures are placed linearly (BumpCore) into a caller-owned
    // device-local TextureHeap, rewound every frame. There is one physical
    // copy; the graph's first-use barrier covers the previous frame still
    // reading the same memory.
    //
    // Objects: the texture + descriptor for each placement is cached, keyed
    // by (descriptor, heap offset). A stable graph places the same textures at
    // the same offsets each frame, so steady state creates nothing.
    //
    // Eviction: an entry not acquired this frame is destroyed at EndFrame().
    // Offsets shift when the graph changes; keeping a stale texture around
    // would let it be reused over memory another texture wrote (aliasing),
    // and its contents would no longer be valid.
    //
    // Main-thread only (texture creation / DescriptorAllocator).
    class RenderGraphRegistry
    {
    public:
        static constexpr u32 k_max_transient_textures = 64;

    public:
        PHX_NO_COPY_NO_MOVE(RenderGraphRegistry);

        RenderGraphRegistry() = default;
        ~RenderGraphRegistry() = default;

        // `heap` and `descriptor_allocator` are owned by the caller and must
        // outlive Shutdown() (and the RHI's deferred-destroy flush).
        void Initialize(rhi::TextureHeap heap, rhi::DescriptorAllocator& descriptor_allocator);
        void Shutdown();

        // Rewinds placement and marks every cached entry unused.
        void BeginFrame();

        // Places a texture for this frame and returns its (cached or newly
        // created) handle. Asserts and returns an invalid handle if the heap
        // or the cache is full.
        [[nodiscard]] TransientTexture Acquire(const rhi::TextureDescriptor& desc);

        // Evicts every entry not acquired since BeginFrame().
        void EndFrame();

    private:
        struct TextureEntry
        {
            rhi::TextureDescriptor desc;
            rhi::TextureHandle     handle;
            rhi::DescriptorIndex   descriptor = rhi::kInvalidDescriptorIndex;
            u64                    offset     = 0;
            bool                   is_used    = false;
        };

        static bool IsCompatible(const rhi::TextureDescriptor& a, const rhi::TextureDescriptor& b);
        void Evict(u32 index);

    private:
        StaticArray<TextureEntry, k_max_transient_textures> m_textures;
        u32 m_texture_count = 0;

        rhi::TextureHeap            m_heap                 = {};
        rhi::DescriptorAllocator*   m_descriptor_allocator = nullptr;
        BumpCore                    m_placement;
    };
}