#pragma once

#include <PhxEngine/Core/Span.h>

#include <hlsl++.h>

namespace phx::renderer
{
    // -- Datat yped
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