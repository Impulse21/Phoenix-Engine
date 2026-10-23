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
        u32 width;
        u32 height;
        rhi::Format format;
        const char* debug_name;
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
            TextureDesc texture;
            BufferDesc buffer;
        };

		ResourceDesc() = default;

        static ResourceDesc Texture(TextureDesc t)
        {
            ResourceDesc d = {};
            d.kind = ResourceKind::Texture;
            d.texture = t;
            return d;
        }
        
        static ResourceDesc Buffer(BufferDesc b)
        {
            ResourceDesc d = {};
            d.kind = ResourceKind::Buffer;
            d.buffer = b;
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