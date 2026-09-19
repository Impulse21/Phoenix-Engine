#pragma once

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/RHI/GpuMemory/DescriptorAllocator.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>

namespace phx::renderer
{
    struct FrameRenderTargets
    {
        rhi::TextureHandle scene_colour;
        rhi::TextureHandle depth;
    };

    class IRenderer
    {
    public:
        virtual bool Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual Span<const FrameRenderTargets> GetOrCreateFrameRenderTargets(u32 width, u32 height) = 0;

        // TODO: Maybe hide this within the RHI.
        virtual rhi::DescriptorAllocator& GetDescriptorAllocator() = 0;
        virtual rhi::TextureAllocator& GetTextureAllocator() = 0;
        
        virtual ~IRenderer() = default;
    };
}