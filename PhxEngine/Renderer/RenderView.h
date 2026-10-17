#pragma once

#include <hlsl++.h>

namespace phx::renderer
{
    struct RenderView
    {    
        u32              kind;
        hlslpp::float4x4 view;
        hlslpp::float4x4 proj;
        hlslpp::float4x4 view_proj;
        u32              viewport[2];   // TODO: Create a rect or extents class
    };
}