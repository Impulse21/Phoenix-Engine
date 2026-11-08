#pragma once

#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/Memory/IHeapAllocator.h>

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

    class RenderGraphRegistry
    {
    public:
        PHX_NO_COPY_NO_MOVE(RenderGraphRegistry);

        RenderGraphRegistry() = default;
        ~RenderGraphRegistry() = default;

        void Initialize();
        void Shutdown();

        // Called at the start of compile - marks all entries aviable.
        void BeginFrame();

        [[nodiscard]] rhi::TextureHandle FindOrCreateTexture(const TextureDesc& desc) const;

        public:
            struct TextureEntry
            {
                TextureDesc desc;
                rhi::TextureHandle handle;
                bool is_used = false;
            };

            static u64 HashTextureDesc(const TextureDesc& desc);

        private:
            TextureEntry* m_textures = nullptr;
            u32 m_texture_count = 0;
            u32 m_texture_capacity = 0;
    };
}