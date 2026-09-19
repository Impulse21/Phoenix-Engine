#pragma once

#include "IRenderer.h"

#include <optional>
namespace phx::renderer
{
    class RendererBase : public IRenderer
    {
    public:
        Span<const FrameRenderTargets> GetOrCreateFrameRenderTargets(u32 width, u32 height) override
        {
            if (!m_frame_render_targets)
            {
                m_frame_render_targets.emplace();
                for (u32 i = 0; i < m_frame_render_targets->size(); ++i)
                {
                    // TODO: Use the new GPU Heap API.
                    // Once I am done that I can officall cleanup my create texture handle code.
                    rhi::TextureHandle colour_target = rhi::CreateTexture({
                        .debug_name             = "colour_target",
                        .format                 = k_colour_buffer_format,
                        .width                  = width, 
                        .height                 = height,
                        .clear_value            = { .colour { 1.0f, 1.0f, 1.0f, 1.0f} },
                        .binding_flags          = rhi::BindingFlags::RenderTarget | rhi::BindingFlags::ShaderResource,
                        .initial_state          = rhi::ResourceStates::RenderTarget,
                    });

                    rhi::TextureHandle depth_target = rhi::CreateTexture({
                        .debug_name             = "depth_target",
                        .format                 = k_depth_buffer_format,
                        .width                  = width, 
                        .height                 = height,
                        .clear_value            = { .depth_stencil = { 0.0f }},
                        .binding_flags          = rhi::BindingFlags::DepthStencil,
                        .initial_state          = rhi::ResourceStates::DepthWrite,
                    });

                    m_frame_render_targets.value()[i] = {
                        .scene_colour = colour_target,
                        .depth = depth_target,
                    };
                }   

                return m_frame_render_targets.value();
            }
        }

    protected:
        static constexpr rhi::Format k_colour_buffer_format = rhi::Format::RGBA16_FLOAT;
        static constexpr rhi::Format k_depth_buffer_format =  rhi::Format::D32;;
        std::optional<std::array<FrameRenderTargets, rhi::MaxFramesInFlight>> m_frame_render_targets;
    };
}