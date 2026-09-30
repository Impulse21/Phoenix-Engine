#pragma once

#include <array>

#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/StaticArray.h>

#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/RHI/GpuMemory/TextureAllocator.h>
#include <PhxEngine/RHI/GpuMemory/DescriptorAllocator.h>

namespace phx::renderer
{
    struct FrameRenderTargets
    {
        rhi::TextureHandle scene_colour;
        rhi::DescriptorIndex scene_colour_index;
        rhi::TextureHandle depth;
        rhi::DescriptorIndex depth_index;
    };

    class HdrRenderTargets
    {
    public:
        static constexpr rhi::Format k_colour_buffer_format = rhi::Format::RGBA16_FLOAT;
        static constexpr rhi::Format k_depth_buffer_format  = rhi::Format::D32;

    public:
        void Initialize(rhi::TextureAllocator& texture_allocator, rhi::DescriptorAllocator& desciptor_allocator)
        {
            m_texture_allocator = &texture_allocator;
            m_desciptor_allocator = &desciptor_allocator;
        }

        Span<const FrameRenderTargets> GetOrCreateFrameRenderTargets(u32 width, u32 height)
        {
            PHX_ASSERT(m_texture_allocator);

            if (!m_created)
            {
                for (u32 i = 0; i < m_placed.Size(); ++i)
                {
                    Placed& placed = m_placed[i];

                    placed.colour = m_texture_allocator->Alloc({
                        .debug_name             = "colour_target",
                        .format                 = k_colour_buffer_format,
                        .width                  = width,
                        .height                 = height,
                        .clear_value            = { .colour { 1.0f, 1.0f, 1.0f, 1.0f} },
                        .binding_flags          = rhi::BindingFlags::RenderTarget | rhi::BindingFlags::ShaderResource,
                        .initial_state          = rhi::ResourceStates::RenderTarget,
                    });

                    placed.colour_index = m_desciptor_allocator->Allocate(placed.colour.handle);

                    placed.depth = m_texture_allocator->Alloc({
                        .debug_name             = "depth_target",
                        .format                 = k_depth_buffer_format,
                        .width                  = width,
                        .height                 = height,
                        .clear_value            = { .depth_stencil = { 0.0f }},
                        .binding_flags          = rhi::BindingFlags::DepthStencil,
                        .initial_state          = rhi::ResourceStates::DepthWrite,
                    });
                    
                    placed.depth_index = m_desciptor_allocator->Allocate(placed.depth.handle);

                    PHX_ASSERT(placed.colour.handle.IsValid() && placed.depth.handle.IsValid());

                    m_targets[i] = {
                        .scene_colour       = placed.colour.handle,
                        .scene_colour_index = placed.colour_index,
                        .depth              = placed.depth.handle,
                        .depth_index        = placed.colour_index,
                    };
                }

                m_created = true;
            }

            return m_targets;
        }

        void DestroyFrameRenderTargets()
        {
            if (!m_created)
                return;

            for (Placed& placed : m_placed)
            {
                m_texture_allocator->Free(placed.colour);
                m_texture_allocator->Free(placed.depth);

                m_desciptor_allocator->Free(placed.colour_index);
                m_desciptor_allocator->Free(placed.depth_index);
            }

            m_targets = {};
            m_created = false;
        }

    private:
        struct Placed
        {
            rhi::PlacedTexture colour;
            rhi::DescriptorIndex colour_index;
            rhi::PlacedTexture depth;
            rhi::DescriptorIndex depth_index;
        };

        rhi::TextureAllocator*      m_texture_allocator = nullptr;
        rhi::DescriptorAllocator*   m_desciptor_allocator = nullptr;
        bool m_created = false;

        StaticArray<Placed, rhi::MaxFramesInFlight>              m_placed;
        StaticArray<FrameRenderTargets, rhi::MaxFramesInFlight>  m_targets;
    };
}
