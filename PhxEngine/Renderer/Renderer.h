#pragma once

#include <PhxEngine/Core/Span.h>

#include <hlsl++.h>

namespace phx::renderer
{
    // -- Data types ---
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
    };


    // -- TODO Frustrum helpers ---
}