#pragma once

#include <PhxEngine/Core/Span.h>

namespace phx
{
    class FrameAllocator;
    class ScratchAllocator;
}

namespace horde
{
    struct RenderPacket
    {
        phx::rhi::PipelineStateHandle pso_handle;
        phx::rhi::GpuRange draw_root;
        u32 instance_count;
    };

    struct VisibileLight
    {
        u32 light_index;
        u32 first_view;
        u32 view_count;
    };
    
    struct RenderView
    {
        u32 Kind;
        hlslpp::float4x4 view;
        hlslpp::float4x4 proj;
        hlslpp::float4x4 view_proj;

        // -- What is visibible to this vbiew ---
        phx::Span<RenderPacket> render_list;
    };


    // -- This is in the app for now, might move up into the runtime eventually ----
    struct FrameData
    {
        phx::FrameAllocator* allocator;
        phx::ScratchAllocator* scratch;

        phx::Span<RenderView> views;
    };
}