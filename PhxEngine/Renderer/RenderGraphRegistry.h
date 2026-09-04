#include <PhxEngine/RHI/RHITypes.h>

#if false
#include <PhxEngine/Memory/IHeapAllocator.h>
#include <PhxEngine/Renderer/RenderGraph.h>

namespace phx::renderer
{
    class RenderGraphRegistry
    {
    public:
        PHX_NO_COPY_NO_MOVE(RenderGraphRegistry);

        RenderGraphRegistry() = default;
        ~RenderGraphRegistry() = default;

        void Initialize(phx::IHeapAllocator* heap_alloc);
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
            IHeapAllocator* m_heap_alloc = nullptr;
    };
}
#endif